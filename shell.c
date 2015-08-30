#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/wait.h>



char *replace_str(char *str, char *orig, char *rep) // replacing ~ with home
{
    static char buffer[4096];
    char *p;
     
    if(!(p = strstr(str, orig)))
    return str;
     
    strncpy(buffer, str, p-str);
    buffer[p-str] = '\0';
     
    sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
     
    return buffer;
}
 

char *trimwhitespace(char *str)
{
  char *end;
  while(isspace(*str)) str++;

  if(*str == 0)  
    return str;

  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  *(end+1) = 0;

  return str;
}

char * readLine ( char *buffer ) //reading line
{
    char *ch = buffer;
    while ((*ch = getchar()) != '\n')
        ch++;
    *ch = 0;
    return buffer;
}

void parse ( char * line, char ** argv )
{
    while (*line != '\0')
    {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';
        *argv++ = line;
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
            line++;
    }
    *argv = '\0';
}

int check_semicolon (char *command ){
	while ( *command ){
		if( *command == ';' )
			return 1;
		*command++;
	}
	return 0;
}


void parseCommandLine ( char *command )
{
    char *args[100];
    parse(command , args);
    if(strncmp("cd",command,strlen(command)) == 0){
    	DIR* dir = opendir(args[1]);
    	if(dir){
    		closedir(dir);
    	}
    	else{
    		printf("Cannot open the directory\n");
    		return;
    	}
    	chdir(args[1]);
    	return;
    }
    int pid = fork();
    int status = 0;
    if (pid != 0)
    {
        waitpid (pid , &status , WUNTRACED);
    }
    else
    {
        if (execvp ( command , args ) == -1) //executing command
		{
            fprintf ( stderr , "command doesn't exist : %s\n" , command );
		    kill (getpid() , SIGTERM);
		}
    }
}

int main ( )
{
    char *command = malloc(1000*sizeof(char));
    char *user = getenv("USER");
    char hostname[1000];
    gethostname(hostname,1000);
    while (1) 
    {
    	char *home = malloc(1000*sizeof(char));
    	strcpy(home,"/home/");
    	home = strcat(home,user); 
    	char cwd[1000];
    	char *curr_dir = getcwd(cwd, sizeof(cwd));
    	char *display = malloc(1000*sizeof(char));
    	strcpy(display,user);
    	display = strcat(display,"@");
    	display = strcat(display,hostname);
    	display = strcat(display,":");
    	display = strcat(display,curr_dir);
        display = strcat(display,"$ ");
        printf("%s",display);
        readLine(command);
        while (command[0] == '\0')
        {
            printf("%s",display);
            readLine(command);
        }
        command = replace_str(command,"~",home);
        if( check_semicolon ){
        	command = trimwhitespace(command);
        	char *pch = strtok( command , ";" );
        	while( pch != NULL ){
        		pch = trimwhitespace(pch);
        		if (strncmp (pch , "quit" , 4) == 0)
            		return 0;
        		if (strncmp (pch , "exit" , 4) == 0)
            		return 0;
        		parseCommandLine(pch);
        		pch = strtok( NULL , ";" );
        	}
        	continue;
        }
        if (strncmp (command , "quit" , 4) == 0)
            return 0;
        if (strncmp (command , "exit" , 4) == 0)
            return 0;
        parseCommandLine(command);
    }
    return 0;
}
