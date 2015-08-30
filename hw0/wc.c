#include <stdio.h>

int main(int argc, char *argv[]) {

	// printf("Personal WC \n");

	int TRUE = 1;
	int FALSE = 0;

	FILE *fp;
	long charCount;
	long lineCount;
	long wordCount;
	int c;
	char *buffer;
	int noWordBefore;


	charCount = 0;
	lineCount = 0;
	wordCount = 0;
	noWordBefore = TRUE;
	buffer = argv[1];

	fp = fopen( argv[1] , "r" );

	if (buffer == 0x0) {
		c = getchar();

		while (c != -1) {
			// printf("%d \n", c);
			if (c == 12) {
				// x0c
				noWordBefore = TRUE;
				c = getchar();
				charCount++;
				continue;
			}
			if (c == 0) {
				// noWordBefore = TRUE;
				c = getchar();
				charCount++;
				continue;
			}
			if (c == 13 || c == 11) {
				// x0b
				noWordBefore = TRUE;
				c = getchar();
				charCount++;
				continue;
			}
			if (c == 10 ) {
				// new line character
				noWordBefore = TRUE;
				lineCount++;
			} else {
				// there is a character (includes spaces)
				if (c == 32) {
					// space
					noWordBefore = TRUE;
				} else {
					if (noWordBefore == TRUE) {
						noWordBefore = FALSE;
						wordCount++;
					}
				}
			}
			charCount++;
			c = getchar();
		}
		printf("%ld %ld %ld \n", lineCount, wordCount, charCount);
	} else {

		c = fgetc(fp);


		while (c != -1) {
			if (c == 0) {
				noWordBefore = TRUE;
				c = getchar();
				charCount++;
				continue;
			}
			if (c == 13) {
				noWordBefore = TRUE;
				c = fgetc(fp);
				charCount++;
				continue;
			}
			if (c == 10) {
				// new line character
				noWordBefore = TRUE;
				lineCount++;
			} else {
				// there is a character (includes spaces)
				if (c == 32) {
					// space
					noWordBefore = TRUE;
				} else {
					if (noWordBefore == TRUE) {
						noWordBefore = FALSE;
						wordCount++;
					}
				}
			}
			charCount++;
			c = fgetc(fp);
		}
		fclose(fp);
		printf("%ld %ld %ld %s \n", lineCount, wordCount, charCount, argv[1]);
	}


    return 0;
}


