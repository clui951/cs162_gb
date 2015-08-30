#include <stdio.h>

int main(int argc, char *argv[]) {

	printf("Personal WC \n");

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

	fp = fopen ( argv[1] , "r" );
	c = fgetc(fp);


	while (c != -1) {
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

	printf("%ld %ld %ld %s \n", lineCount, wordCount, charCount, argv[1]);

    return 0;
}


