/*************************************************
	* C program to count no of lines, words and 	 *
	* characters in a file.			 *
	*************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#define FILEPATH "/tmp/CSCI4730/books"

typedef struct count_t {
		int linecount;
		int wordcount;
		int charcount;
} count_t;

count_t word_count(char *file)
{
		FILE *fp;
		char ch;

		count_t count;
		// Initialize counter variables
		count.linecount = 0;
		count.wordcount = 0;
		count.charcount = 0;

		printf("[pid %d] read: %s\n", getpid(), file);
		// Open file in read-only mode
		fp = fopen(file, "r");
		
		//sleep(10);
		// If file opened successfully, then write the string to file
		if ( fp )
		{
				//Repeat until End Of File character is reached.	
				while ((ch=getc(fp)) != EOF) {
						// Increment character count if NOT new line or space
						if (ch != ' ' && ch != '\n') { ++count.charcount; }

						// Increment word count if new line or space character
						if (ch == ' ' || ch == '\n') { ++count.wordcount; }

						// Increment line count if new line character
						if (ch == '\n') { ++count.linecount; }
				}

				if (count.charcount > 0) {
						++count.linecount;
						++count.wordcount;
				}

				fclose(fp);
		}
		else
		{
				printf("Failed to open the file: %s\n", file);
		}

		return count;
}

#define MAX_PROC 100

int main(int argc, char **argv)
{
		int numFiles, numProcs;
		char filename[100];
		int pid[MAX_PROC];
		int count[10];   
		int extraFiles;  // numFiles - ( numProcs * eachProcs)
		int procsId;
		int numText;
		int eachProcs;
		int currentFiles;
		int i,j,k,m;
		int wstatus;
		count_t total, tmp;

		if(argc < 3) {
				printf("usage: wc <# of files to count(1-10)> <# of processes to fork(1-10)\n");
				return 0;
		}

		numFiles = atoi(argv[1]);
		numProcs = atoi(argv[2]);
		if(numFiles <= 0 || numFiles > 10 || numProcs <= 0 || numProcs > 10) {
				printf("usage: wc <# of files to count(1-10)> <# of processes to fork(1-10)\n");
				return 0;
		}

		total.linecount = 0;
		total.wordcount = 0;
		total.charcount = 0;

		printf("counting %d files in %d processes.\n", numFiles, numProcs);


        int pipefd[2];
        pid_t cpid;

        if (pipe(pipefd) == -1){
        	perror("Failed to create pipe\n");
        	exit (EXIT_FAILURE);
        }
 
        //write initial value to pipe
        write (pipefd[1], &total, sizeof(count_t));
  

		eachProcs = numFiles/numProcs;
		extraFiles = numFiles - (eachProcs * numProcs);
		procsId = 0;


        // assign exact division files to each child process
		for ( k = 0; k < numProcs; k++){
			count[k] = eachProcs;

		} 

		// assign extra files to child processes
		while(extraFiles != 0){
			count[procsId] = count[procsId] + 1;
			procsId++;
			extraFiles = extraFiles - 1;
		}

		currentFiles = 0;

		for ( m = 0; m < numProcs; m++){
			currentFiles = currentFiles + count[m];
			if (m == 0)
			    printf("Child Proc %d, read %d files (0 ~ %d)\n",m,count[m],count[m] - 1);
		    else
		    	printf("Child Proc %d, read %d files (%d ~ %d)\n",m,count[m],currentFiles-count[m],currentFiles-1);

		}


        numText = 0;


		for ( i = 0; i < numProcs; i++){

			cpid = fork();

			if( i != 0 ){
				numText = numText + count[i-1];
			}

			if ( cpid == -1 ){
				perror("Failed to create a child process\n");
				exit(EXIT_FAILURE);
			}
			if ( cpid == 0 ){

				for( j = 0; j < count[i]; j++){

                    
				    sprintf(filename, "%s/text.%02d", FILEPATH, numText);
				    numText = numText + 1;

				    //printf("read: %s\n", filename);
				    tmp = word_count(filename);

				    read ( pipefd[0], &total, sizeof(count_t));


				    total.charcount += tmp.charcount;
				    total.linecount += tmp.linecount;
				    total.wordcount += tmp.wordcount;

				    
				    write ( pipefd[1], &total, sizeof(count_t));


			    }

			    printf("[pid %d] send the result to the parent %d.\n", getpid(),getppid() );
			    exit (EXIT_SUCCESS);

			}

			pid[i] = cpid; // record cpid

		}

        
		for(i = 0; i < numProcs; i++)
		{
				waitpid(pid[i], &wstatus, 0);
				if(WIFEXITED(wstatus)) printf("The child process %d terminated normally. The Exit status %d\n", pid[i], WEXITSTATUS(wstatus));
				if(WIFSIGNALED(wstatus)) printf("The child process %d terminated by a signal %d.\n", pid[i], WTERMSIG(wstatus));
		}
		

		//while(-1 != wait(NULL));

		close(pipefd[1]);

		read (pipefd[0], &total, sizeof(count_t));

		close(pipefd[0]);




		printf("\n=========================================\n");
		printf("Total Lines : %d \n", total.linecount);
		printf("Total Words : %d \n", total.wordcount);
		printf("Total Characters : %d \n", total.charcount);
		printf("=========================================\n");

		return(0);
}
