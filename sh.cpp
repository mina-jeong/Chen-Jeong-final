#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <libgen.h>
         
using namespace std;

void env(char **e);
void io(char **args);
int checkAmp(char **args);

extern char **environ; 
pid_t pid;            
int status;            
int in, out, a, b, c; 
char *inFile, *outFile; 


/*
 * @param char ** args- pointer to a pointer to a char
 *
 * This function goes through argv and checks whether there is a 
 * I/O redirection and if there is which one.
 */
void io(char ** argv){
  // reset input and output and append
  a = 0;
  b = 0;
  c = 0;
  int i = 0;
  while(argv[i] != NULL){
    if (!strcmp(argv[i], "<")){           //input redirection
      strcpy(argv[i], "\0");
      inFile = argv[i+1];
      a = 1;
    }
    else if(!strcmp(argv[i], ">>")){     //output redirection with appending
      outFile = argv[i+1];
      argv[i] = NULL;
      c = 1;
      break;
    }
    else if(!strcmp(argv[i], ">")){      //output redirection
      outFile = argv[i+1];
      argv[i] = NULL;
      b = 1;
      break;
    }
    i++;
  }
}

/*
 * @param argv
 *
 * Makes sure that the end of argv ends with NULL
 *
 */
int checkAmp(char **argv){
  int i = 0;
  int found = 0;
  while(argv[i] != NULL){
    if (!strcmp(argv[i], "&")){
      found = 1;
      argv[i] = NULL; //Replace & with NULL
    }
    i++;
  }
  return found;
}

/*
 * @param char **e
 * 
 * Gets the environmental variables and prints it on the screen
 *
 */
void env(char **e){
  FILE *fd;
  char **env = e;
  
  if (b == 1){
    fd = fopen(outFile, "w");
  }
  else if (c == 1){
    fd = fopen(outFile, "a");
  }
  
  if (b == 1 || c == 1){
    while(*env){
      cout << fd << *env++ << endl;     //append fd and print
    }
    fclose(fd);
  }
  else{
    while(*env){
      cout << *env++ << endl;           //else print
      
    }
  }  
}

// the main function 
int main(int argc, char ** argv){
  char buf[1024];
  char * args[20];
  char ** arg;
  const char * path;
  char r[PATH_MAX];
  ssize_t c = readlink("/proc/self/exe", r, PATH_MAX);
  if (c != -1) path = dirname(r);
  const char * prompt = path;
  int found = 0;
  int status;

  if(argc > 1) {
    freopen(argv[1], "r", stdin);
  }

  while(!feof(stdin)){
    cout << "1730sh:" << path << "$ ";

    if(fgets(buf, 1024, stdin)){
      arg = args;
      *arg++ = strtok(buf," \t\n");

      while ((*arg++ = strtok(NULL, " \t\n")));

      io(args); //I/O Redirection
      found = checkAmp(args); //Make sure last space is null

      if (args[0]) {
        // Input redirection
        if (a == 1){
          if(!access(inFile, R_OK)){ //Access ok?
            freopen(inFile, "r", stdin); // Redirect stdin with file
          }//if access
        }//if input=1
	
	if(!strcmp(args[0], "export")){ //Add and change environment variables using export
	  char * v = args[1];
	  char *val = getenv(v);
	  char *string;
	 
	  val = argv[2];
	  string = (char*)malloc(strlen(v));
	  if(!string){
	    cout << stderr << "memory error";
	      exit(1);
            }
	  
	  strcpy(string,v);
	  strcat(string," ");
	  printf("",string);
	  if(putenv(string)!=0){
	    fprintf(stderr,"putenv fail ");
	    free(string);
	    exit(1);
	  }
	  val = getenv(v);
	  continue;
	}

        //get the environment variables of the shell
        if (!strcmp(args[0], "env")) {
          env(environ); 
          continue;
        }//if environ
	
	if (!strcmp(args[0],"echo")) { 
          pid = getpid(); // get process id
	  
          if((pid = fork()) == -1){
	    perror("FORK ERROR");
	    abort();
	  }
	  else if(pid == 0){
	    setenv("parent", getenv("shell"), 1); 
	    
	    //i/o redirection for output
	    if(b == 1)
	      freopen(outFile, "w", stdout);
	    
	    //i/o redirection for input
	    else if(a == 1)
	      freopen(outFile, "a+", stdout);
	    
	    if(execvp (args[0], args) == -1){
	      perror("EXEC CHILD ERROR");
	      abort();
	    }
	  }
	  else{                
	    if (!found) 
	      waitpid(pid, &status, WUNTRACED); //calls wait
	  }
          continue;
	}
	
        if (!strcmp(args[0],"exit")) { 
          break; //break the loop so the program returns and ends
        }

	else{
	  pid = getpid();   
          if((pid = fork ())== -1) { 
	    perror("FORK ERROR");
	    abort();
	  }
	  else if(pid == 0){
	    setenv("parent", getenv("shell"), 1); //set parent
	    if(b == 1)
	      freopen(outFile, "w", stdout);
	    else if(c == 1)
	      freopen(outFile, "a+", stdout); 
	    
	    if(execvp (args[0], args) == -1){
	      perror("EXEC CHILD ERROR");
	      abort();
	    } //execute in child thread
	  }else{                
	    if (!found) //determine background execution wait (&)
	      waitpid(pid, &status, WUNTRACED);
	  }	  
	}
	continue;
      }//if args[0]
    }//if fgets
  }//while feof
  return 0;
}//main
