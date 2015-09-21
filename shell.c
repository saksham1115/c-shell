#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HARAKIRI 1143

int ampercent = 0;


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

int check_ampercent(char *command){
	while ( *command ){
		if( *command == '&' )
			return 1;
		*command++;
	}
	return 0;
}

int check_semicolon (char *command ){
	while ( *command ){
		if( *command == ';' )
			return 1;
		*command++;
	}
	return 0;
}

char * readLine ( char *buffer )
{
    char *ch = buffer;
    while ((*ch = getchar()) != '\n')
        ch++;
    *ch = 0;
    return buffer;
}

void parse ( char * line, char ** argv )
{
    while (*line != '\0' && *line != '>' && *line != '<')
    {
        while (*line == ' ' || *line == '\t' || *line == '\n')
            *line++ = '\0';
        *argv++ = line;
        while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n')
            line++;
    }
    *argv = '\0';
}

int run_process ( char *command , char ** args  , int files , char * inputFile , char * outputFile )
{
    int pid = fork();
    int status = 0;
    if (pid != 0)
    {
    	if(ampercent == 0){
        	waitpid (pid , &status , WUNTRACED);
        	return 0;
        }
    }
    else
    {
    	if(ampercent)
    		setpgid(0,0);
        int fd_in = 0;
        int fd_out = 1;
        switch (files & 1)
        {
            case 0:
                break;
            case 1:
                if (inputFile == NULL)
                {
                    fprintf(stderr,"INVALID FILE!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                if (access ( inputFile , F_OK ))
                {
                    fprintf(stderr,"INVALID FILE!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                fd_in = open ( inputFile , O_RDWR );
                if (fd_in == -1)
                {
                    fprintf(stderr, "PERMISSIONS DENIED!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                dup2(fd_in , STDIN_FILENO);
                close (fd_in);
                break;
        }
        switch ((files & 2) >> 1)
        {
            case 0:
                break;
            case 1:
                if (outputFile == NULL)
                {
                    fprintf(stderr,"INVALID FILE!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                fd_out = open ( outputFile , O_RDWR | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWOTH );
                if (fd_out == -1)
                {
                    fprintf(stderr , "PERMISSIONS DENIED!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                dup2(fd_out , STDOUT_FILENO);
                close (fd_out);
                break;
        }
        switch ((files & 4) >> 2)
        {
            case 0:
                break;
            case 1:
                if (outputFile == NULL)
                {
                    fprintf(stderr , "INVALID FILE!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                fd_out = open ( outputFile , O_RDWR | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWGRP | S_IWOTH );
                if (fd_out == -1)
                {
                    fprintf(stderr , "PERMISSIONS DENIED!!\n");
                    kill (getpid() , SIGTERM);
                    return HARAKIRI; // Because Kill Yourself.
                }
                dup2(fd_out , STDOUT_FILENO);
                close (fd_out);
                break;
        }
        if (execvp ( command , args ) < 0)
		{
            fprintf ( stderr , "COULD NOT FIND COMMAND : %s\n" , command );
		    kill (getpid() , SIGTERM);
		    return HARAKIRI;  // Because Kill Yourself.
		}
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
    char * inputFile = NULL;
    char * outputFile = NULL;
    int flags = 0;
    int len;
    for (len = 0 ; args[len] ; len++)
        ;
    int i;
    for (i = 0 ; args[i] ; i++)
    {
        if (args[i][0] == '<')
        {
            if (i + 1 < len)
                inputFile = args[i + 1];
            else
                inputFile = NULL;
            flags |= 1;
            args[i] = NULL;
            i++;
        }
        if (args[i][0] == '>')
        {
            flags |= 2;
            if (strlen(args[i]) == 2 && args[i][1] == '>')
            {
                flags |= 4;
                flags &= ~2;
            }
            if (i + 1 < len)
                outputFile = args[i + 1];
            else
                outputFile = NULL;
            args[i] = NULL;
            i++;
        }
    }
    run_process ( command , args , flags , inputFile , outputFile );
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
        		ampercent = 0;
        		ampercent = check_ampercent(pch);
        		pch = replace_str(pch,"&"," ");
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

