#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "kvconstants.h"
#include "kvmessage.h"
#include "index.h"
#include "md5.h"
#include "socket_server.h"
#include "time.h"
#include "tpcleader.h"

/* Initializes a tpcleader. Will return 0 if successful, or a negative error
 * code if not. FOLLOWER_CAPACITY indicates the maximum number of followers that
 * the leader will support. REDUNDANCY is the number of replicas (followers) that
 * each key will be stored in. */
int tpcleader_init(tpcleader_t *leader, unsigned int follower_capacity,
    unsigned int redundancy) {
  int ret;
  ret = pthread_rwlock_init(&leader->follower_lock, NULL);
  if (ret < 0) return ret;

  leader->follower_count = 0;
  leader->follower_capacity = follower_capacity;
  if (redundancy > follower_capacity) {
    leader->redundancy = follower_capacity;
  } else {
    leader->redundancy = redundancy;
  }
  leader->followers_head = NULL;
  leader->handle = tpcleader_handle;
  return 0;
}

/* Handles an incoming kvrequest REQ, and populates RES as a response. REQ and
 * RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively. Assigns an ID to the follower by hashing a string in the format
 * PORT:HOSTNAME, then tries to add its info to the LEADER's list of followers. If
 * the follower is already in the list, do nothing (success).  There can never be
 * more followers than the LEADER's follower_capacity.  RES will be a SUCCESS if
 * registration succeeds, or an error otherwise.
 */
void tpcleader_register(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  if (leader->follower_count == leader->follower_capacity) {
    res->type = ERROR;
    alloc_msg(res->body, ERRMSG_FOLLOWER_CAPACITY);
    return;
  }

  tpcfollower_t *new_follower = malloc(sizeof(tpcfollower_t));
  if (!new_follower) fatal_malloc();

  new_follower->host = malloc(strlen(req->key) + 1);
  if (!new_follower->host) fatal_malloc();
  strcpy(new_follower->host, req->key);
  new_follower->port = atoi(req->val);
  char address[strlen(new_follower->host) + strlen(req->val)];
  sprintf(address, "%s:%s", req->val, new_follower->host);
  new_follower->id = strhash64(address);
  new_follower->prev = new_follower;
  new_follower->next = new_follower;

  res->type = SUCCESS;
  pthread_rwlock_wrlock(&leader->follower_lock);
  if (!leader->followers_head) {
    leader->followers_head = new_follower;
    leader->follower_count++;
    goto end;
  }
  tpcfollower_t *first_follower = leader->followers_head;
  tpcfollower_t *curr_follower = first_follower;
  do {
    if (curr_follower->id > new_follower->id) {
      new_follower->next = curr_follower;
      new_follower->prev = curr_follower->prev;
      curr_follower->prev = new_follower;
      new_follower->prev->next = new_follower;
      if (curr_follower == first_follower)
        leader->followers_head = new_follower;
      leader->follower_count++;
      goto end;
    } else if (curr_follower->id == new_follower->id) {
      goto end;
    }
    curr_follower = curr_follower->next;
  } while (curr_follower != first_follower);
  /* New follower has highest ID. */
  new_follower->next = leader->followers_head;
  new_follower->prev = leader->followers_head->prev;
  leader->followers_head->prev = new_follower;
  new_follower->prev->next = new_follower;
  leader->follower_count++;

end:
  pthread_rwlock_unlock(&leader->follower_lock);
  return;
}

/* Hashes KEY and finds the first follower that should contain it.
 * It should return the first follower whose ID is greater than the
 * KEY's hash, and the one with lowest ID if none matches the
 * requirement. Returns NULL if we're not yet at our follower capacity.
 */
tpcfollower_t *tpcleader_get_primary(tpcleader_t *leader, char *key) {
  if (leader->follower_count < leader->follower_capacity)
    return NULL;
  uint64_t hash = strhash64(key);
  pthread_rwlock_wrlock(&leader->follower_lock);
  tpcfollower_t *curr_follower = leader->followers_head;
  do {
    if (curr_follower->id > hash) {
      pthread_rwlock_unlock(&leader->follower_lock);
      return curr_follower;
    }
    curr_follower = curr_follower->next;
  } while (curr_follower != leader->followers_head);
  curr_follower = leader->followers_head;
  pthread_rwlock_unlock(&leader->follower_lock);
  return curr_follower;
}

/* Returns the follower whose ID comes after PREDECESSOR's, sorted
 * in increasing order.
 */
tpcfollower_t *tpcleader_get_successor(tpcleader_t *leader,
    tpcfollower_t *predecessor) {
  pthread_rwlock_wrlock(&leader->follower_lock);
  tpcfollower_t *result = predecessor->next;
  pthread_rwlock_unlock(&leader->follower_lock);
  return result;
}

/* Handles an incoming GET request REQ, and populates response RES. REQ and
 * RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively.
 */
void tpcleader_handle_get(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  /* TODO: Implement me! */
  char *req_key = req->key;
  int r_val = leader->redundancy;
  tpcfollower_t *curr_follower = tpcleader_get_primary(leader, req_key);

  int flag = 0;
  while (r_val > 0) {
    int socketfd = connect_to(curr_follower->host, curr_follower->port, 2);
    int req_send = kvrequest_send(req, socketfd);
    if (socketfd == -1 || req_send == -1) {       // CONNECTION FAILED!!!
      curr_follower = curr_follower->next;
    } else {
      kvresponse_t *response = kvresponse_recieve(socketfd);
      if (response == NULL || response->type != GETRESP) {
        curr_follower = curr_follower->next;
      }
      if (response->type == GETRESP) {
        flag = 1;     // FOUND A VAL TO RETURN!!!
        res->type = GETRESP;
        alloc_msg(res->body, response->body);
        close(socketfd);
        break;
      }
    }
    close(socketfd);
    r_val = r_val - 1;
  }
  if (flag == 0) {
    res->type = ERROR;
    alloc_msg(res->body, ERRMSG_NO_KEY);
  }    
}

/* Handles an incoming TPC request REQ, and populates RES as a response.
 * REQ and RES both must point to valid kvrequest_t and kvrespont_t structs,
 * respectively.
 *
 * Implements the TPC algorithm, polling all the followers for a vote first and
 * sending a COMMIT or ABORT message in the second phase.  Must wait for an ACK
 * from every follower after sending the second phase messages.
 */
void tpcleader_handle_tpc(tpcleader_t *leader, kvrequest_t *req, kvresponse_t *res) {
  if (leader->follower_count != leader->follower_capacity) {
    res->type = ERROR;
    alloc_msg(res->body, ERRMSG_NOT_AT_CAPACITY);
    return;
  }

  char *req_key = req->key;
  int r_val = leader->redundancy;
  tpcfollower_t *curr_follower = tpcleader_get_primary(leader, req_key);
  int sockfd;
  int abortBool = 0;

  // PHASE 1
  while (r_val > 0) {
    r_val = r_val - 1;
    sockfd = connect_to(curr_follower->host, curr_follower->port, 1);
    if (sockfd != -1) {
      kvrequest_send(req, sockfd);
      kvresponse_t *follower_response = kvresponse_recieve(sockfd);
      if (follower_response == NULL || follower_response->type != VOTE) {  
        abortBool = abortBool + 1;      
      } else {
        char *votebody = follower_response->body;
        if (strcmp(votebody, MSG_COMMIT) != 0) {
          abortBool = abortBool + 1;
        } else {
          printf("Phase 1: commit received\n");
        }
      }
    } else {
      printf("Phase 1: no connect\n");
      abortBool = abortBool + 1;
    }
    curr_follower = tpcleader_get_successor(leader, curr_follower);
    close(sockfd);
  }
  // END PHASE 1
  r_val = leader->redundancy;
  curr_follower = tpcleader_get_primary(leader, req_key);

  if (abortBool > 0) {
    req->type = ABORT;
    printf("ABORT\n");
  } else {
    req->type = COMMIT;
    printf("COMMIT\n");
  }
  
  // PHASE 2
  while (r_val > 0) {
    r_val = r_val - 1;
    int acked = 0;
    int counter = 0;
    while (acked == 0) {
      // sleep(1);           // will pass tolerance, but other tests time out
      usleep(60050);
      // counter = counter + 1;
      // if (counter % 5000 == 0) {
      //   sleep(1);
      // }
      sockfd = connect_to(curr_follower->host, curr_follower->port, 1);
      if (sockfd != -1) {
        kvrequest_send(req, sockfd);
        kvresponse_t *follower_response = kvresponse_recieve(sockfd);
        if (follower_response != NULL) {
          if (follower_response->type == ACK) {    
            printf("Phase 2: ACKED\n");
            acked = 1;
          }
        }
        // close(sockfd); 
      } else {
        // if (counter % 5000 == 0) {
        //   printf("connect_to failed: %s %d  on sockfd: %d\n", curr_follower->host, curr_follower->port, sockfd);
        // }
      }
      close(sockfd);
    }
    // curr_follower = curr_follower->next;
    curr_follower = tpcleader_get_successor(leader, curr_follower);
  }
  // END PHASE 2
  
  if (abortBool > 0) {
    res->type = ERROR;
    alloc_msg(res->body, ERRMSG_GENERIC_ERROR);
  } else {
    res->type = SUCCESS;
  }
}


/* Generic entrypoint for this LEADER. Takes in a socket on SOCKFD, which
 * should already be connected to an incoming request. Processes the request
 * and sends back a response message.  This should call out to the appropriate
 * internal handler. */
void tpcleader_handle(tpcleader_t *leader, int sockfd) {
  kvresponse_t *res = calloc(1, sizeof(kvresponse_t));

  kvrequest_t *req;
  req = kvrequest_recieve(sockfd);

  do {
    if (!req) {
      res->type = ERROR;
      alloc_msg(res->body, ERRMSG_INVALID_REQUEST);
    } else if (req->type == INDEX) {
      index_send(sockfd, 1);
      break;
    } else if (req->type == REGISTER) {
      tpcleader_register(leader, req, res);
    } else if (req->type == GETREQ) {
      tpcleader_handle_get(leader, req, res);
    } else {
      tpcleader_handle_tpc(leader, req, res);
    }
    kvresponse_send(res, sockfd);
  } while (0);

  kvrequest_free(req);
  kvresponse_free(res);
}
