#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<termios.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<pwd.h>
#include<ctype.h>
#include<signal.h>
#include<stdlib.h>
#define size 100
char home[size];				//storing the path of the home directory
char name[32770][100];

typedef struct queue{
     int pid;					//structure for storing the name and process id of the background process
     char name[size][size];
}queue;

queue q[size];					//an array of the structure of type queue

int end=-1;					//(end+1) are the total number of elements in queue
int fore_id=-1;					//fore_id stores the process id of the current foreground process

//function made for executing pinfo command
void process_info(char **arguements,int n)
{
     size_t len=0;
     int pid=getpid();				//getting the pid of the a.out(has a meaning if no arguements are given)
     if(n>1)
	  pid=atoi(arguements[1]);		//in case an arguement is given
     printf("pid -- %d\n",pid);			
     char *status=NULL;
     char *memory=NULL;
     char executable_path[size]="";
     char path[size];
     sprintf(path,"/proc/%d/status",pid);	//path storing the path of the status file of the process id
     FILE*f=fopen(path,"r");			//opening the status file for Vmsize and status
     char path1[100];
     sprintf(path1,"/proc/%d/exe",pid);		//path storing the path of the exe file of the process id
     getline(&status,&len,f);			
     getline(&status,&len,f);			//getting the status of the process from 2nd line of the status file	
     int bytes_read;
     bytes_read=readlink(path1,executable_path,100); //reading the link from the exe file and length of the output stored in bytes_read
     executable_path[bytes_read]='\0';
     printf("Process Status -- %s",status+7);
     while((getline(&memory,&len,f))!=-1)
     {
	  if(strncmp(memory,"VmSize",6)==0)	//searching for the VmSize in status file
	  {
	       printf("memory -- %s",memory+12);
	       break;
	  }
     }
     printf("Executable Path -- %s\n",executable_path);
     return;
}



//printing command line after every step
void print_commandline()
{
     char host[size];
     gethostname(host,size);
     char current_dir[size],y[size];
     struct passwd *x;
     int p=geteuid();
     x=getpwuid(p);
     getcwd(current_dir,size);
     if(strcmp(current_dir,home)==0)			//if current directory is the home directory
     {
	  sprintf(y,"%s@%s:~> ",x->pw_name,host);
	  write(1,y,strlen(y));
     }
     else if(strlen(current_dir)<strlen(home))		//if the current directory is the ancestor of the home directory
     {
	  sprintf(y,"%s@%s:%s> ",x->pw_name,host,current_dir);
	  write(1,y,strlen(y));
     }
     else						//if the current directory is the predecessor of the home directory
     {
	  int i,len=strlen(home)-1,k=0;
	  char path[size];
	  for(i=len+1;current_dir[i]!='\0';i++)
	       path[k++]=current_dir[i];
	  path[k]='\0';
	  sprintf(y,"%s@%s:~%s> ",x->pw_name,host,path);
	  write(1,y,strlen(y));
     }
}



//breking the command separating the arguements
int break_command(char *command,char**arguements)
{
     int i=0,j=0,number=0;
     while(*command!='\0')						
     {
	  while(*command==' '  || *command=='\t' || *command=='\n')	//all sppaces get initialised to '\0'
	       *command++='\0';
	  *arguements++=command;					//argurments are allocated the address of the command
	  number++;
	  while(!(*command==' ' || *command=='\t' || *command=='\n' || *command=='\0' ))//simply moving the pointer in non-space characters
	       command++;
     }
     *arguements='\0';							//last arguement is intialised to '\0' as per the syntax of execvp
     return number;							//number contains the number of arguements
}



//inserting into the queue containing pid and arguements
void insert(int pid,char **arguements)
{
     q[++end].pid=pid;
     int i=0;
     while(arguements[i]!='\0')
     {
	  strcpy(q[end].name[i],arguements[i]);				//copying the arguements into queue
	  i++;
     }
     strcpy(q[end].name[i],"\0");
}



//deleting from the queue containing pid and arguements
void delete_queue(int pid)
{
     int i=0,j;
     for(i=0;i<=end;i++)
	  if(q[i].pid==pid)						//finding the particular pid element
	       break;
     for(j=i+1;j<=end;j++)
	  q[j-1]=q[j];							//shifting the whole queue by one step backward
     end--;
}




//function for executing all the commands except some user definned commands and cd
void execute_command(char*command,char**arguements,int arg)
{
     int i;
     int status;
     pid_t pid;
     pid=fork();     //creating a process
     strcpy(name[pid],arguements[0]);
     fore_id=pid;
     if(arg==1)								//if arg==1 then the process needs to performed in foreground
     {
	  int x=waitpid(fore_id,&status,WUNTRACED);			//waiting of the parent for the child process to complete
	  if(WIFSTOPPED(status))					//inserting into the queue
	       insert(fore_id,arguements);
     }
     if(arg==0 && pid>0)						//background process storing the id in the queue
	  insert(pid,arguements);
     if(pid==0)								//child process
     {
	  if(execvp(*arguements,arguements)<0)				//executing command and checking for error
	       printf("No such command found\n");
	  _exit(0);
     }
     return;
}




//signal handler for various signals
void sig_handler(int signo)
{
     if (signo == SIGINT)
	  printf("received SIGINT\n");
     else if (signo == SIGQUIT)
	  printf("received SIGINT\n");
     else if (signo == SIGUSR1)
	  printf("received SIGUSR1\n");
     else if (signo == SIGTSTP)
     {
	  printf("received SIGTSTP\n");
	  printf("%d\n",fore_id);
	  if(fore_id!=-1)
	       kill(fore_id,SIGTSTP);  
     }
     else if(signo==SIGCHLD);
     // printf("received SIGCHLD\n");
}




//kjob implemented here
//looping on kjob implements overkill
void kill_job(int job_id,int signal)
{
     int i=0;
     kill(q[job_id-1].pid,signal);
}


//implementing output redirection
void output_redirection(char ** arguements,int n)
{
     int i;
     int fd;
     for(i=0;i<n;i++)
	  if(strcmp(arguements[i],">")==0)			//finding the position of ">"
	  {
	       fd=open(arguements[i+1],O_CREAT | O_WRONLY,0777);
	       arguements[i]='\0';
	       break;
	  }
     int stdout_original=dup(1);				//saving the original stdout
     dup2(fd,1);						//redirecting stdout to the fd
     execute_command(*arguements,arguements,1);
     dup2(stdout_original,1);					//restoring the original stdout
}



//implementing input redirection
void input_redirection(char ** arguements,int n)
{
     int i;
     int fd;
     for(i=0;i<n;i++)
	  if(strcmp(arguements[i],"<")==0)			//finding the position of "<"
	  {
	       fd=open(arguements[i+1],O_RDONLY);
	       arguements[i]='\0';
	       break;
	  }
     int stdin_original=dup(0);					//saving the original stdin
     dup2(fd,0);						//redirecting the stdin to fd
     execute_command(*arguements,arguements,1);
     dup2(stdin_original,0);					//restoring thr original stdin
}


int fd[20000][2];						//declaring array for pipes
//fd[i][0] corresponds to read end
//fd[i][1] corresponds to write end




int main()
{
     char command[size],c;
     char *arguements[size];
     int k=0,i,j;
     char ch[size];
     int status,pid;
     getcwd(home,size);					//getting the homedirectory as the directory from which the shelll is invoked
     while(1)
     {
	  if (signal(SIGINT, sig_handler) == SIG_ERR)
	       printf("\ncan't catch SIGINT\n");
	  if (signal(SIGQUIT, sig_handler) == SIG_ERR)
	       printf("\ncan't catch SIGQUIT\n");
	  if (signal(SIGUSR1, sig_handler) == SIG_ERR)
	       printf("\ncan't catch SIGUSR1\n");
	  if (signal(SIGTSTP, sig_handler) == SIG_ERR)
	       printf("\ncan't catch SIGTSTP\n");
	  if(signal(SIGCHLD,sig_handler)==SIG_ERR)
	       printf("\ncan't catch SIGCHLD\n");
	  pid=waitpid(-1, &status, WNOHANG);		//checking for all background processes(whether anyone has exited or not)
	  while(pid>0)
	  {
	       delete_queue(pid);	       //deleting from the queue the killed process
	       //    if(WIFEXITED(status))
	       printf("%s with pid %d exited normally\n",name[pid], pid);
	       if(WIFSIGNALED(status))
		    printf("%s with pid %d exited By Signal Number %d\n",name[pid], pid,WTERMSIG(status));
	       write(1,ch,strlen(ch));
	       pid=waitpid(-1, &status, WNOHANG);
	  }
	  k=0;
	  print_commandline();				//printing command line
	  read(0,&c,1);
	  command[k++]=c;
	  while(c!='\n')
	  {
	       read(0,&c,1);				//taking input character by character
	       command[k++]=c;
	  }
	  command[k-1]='\0';
	  if(command[0]=='\0')				//if no command is entered
	       continue;
	  int len=strlen(command);
	  for(i=len-1;i>=0;i--)
	       if(command[i]==' ' || command[i]=='\t')		//removing the extra spaces/tabs at last
		    command[i]='\0';
	       else
		    break;
	  int n=break_command(command,arguements);		//breaking the command into arguements
	  int ipflag=0,opflag=0,ipdir,opdir,npipes=0;
	  int pipe_position[100];
	  int background=0,append_flag=0,apdir;
	  pipe_position[npipes++]=-1;
	  for(i=0;i<n;i++)
	       if(strcmp(arguements[i],">")==0)
	       {
		    opflag=1;						//setting the output flag and storing the position of ">"
		    //  arguements[i]='\0';
		    opdir=i;
	       }
	       else if(strcmp(arguements[i],"<")==0)
	       {
		    ipdir=i;						//setting the input flag and storing the position of "<"
		    //   arguements[i]='\0';
		    ipflag=1;
	       }
	       else if(strcmp(arguements[i],"|")==0)
	       {
		    pipe_position[npipes++]=i;				//calclating number of pipes and saving the position of each pipe
		    arguements[i]='\0';
	       }
	       else if(strcmp(arguements[i],">>")==0)
	       {
		    append_flag=1;					//setting the append flag and saving the position of ">>"
		    apdir=i;
	       }
	  if(strcmp(arguements[n-1],"&")==0)
	  {
	       //      printf("GONE\n");
	       background=1;						//checking if "&" is there or not
	 //      arguements[n-1]='\0';
	  }
	  npipes--;
	  k=0;
	  int level=1;
	  int savein=dup(0);						//saving the oroginal stdin and stdout 
	  int saveout=dup(1);
	  if(npipes>0)
	  {
	       if(background==1)
		    arguements[n-1]='\0';
	       for(i=0;i<=npipes+1;i++)
		    pipe(fd[i]);					//declaring pipes
	       i=0;

	       //executing the first command i.e. command before the first pipe
	       if(fork()==0)
	       {
		    //if there is input redirection specified
		    if(ipflag==1)				
		    {
			 int fd0=open(arguements[ipdir+1],O_RDONLY);		//opening the input file in readonly mode
			 arguements[ipdir]='\0';
			 dup2(fd0,0);						//redirecting the input to the fd0
			 ipflag=0;
		    }
		    dup2(fd[0][1],1);						//putting the output in the first pipe
		    for(i=0;i<=npipes;i++)
		    {
			 close(fd[i][0]);					//cloding all the pipes
			 close(fd[i][1]);
		    }
		    if(execvp(arguements[pipe_position[0]+1],&arguements[pipe_position[0]+1])<0)
		    {;
		    }
		    //executing the command
		    // 	    execvp(&arguements[pipe_position[0]+1]);

	       }
	       else
	       {

		    //executing all the command except the first and the last
		    for(i=1;i<=(npipes-1);i++)
		    {
			 if(fork()==0)
			 {
			      dup2(fd[i-1][0],0);			//taking input from the previous pipe
			      dup2(fd[i][1],1);				//redirecting the output to the next pipe
			      for(k=0;k<=npipes;k++)
			      {
				   close(fd[k][0]);			//closing all the pipes
				   close(fd[k][1]);
			      }
			      execvp(arguements[pipe_position[i]+1],&arguements[pipe_position[i]+1]);	//executing the command
			 }
		    }
	       }

	       //executing the last comamnd i.e. command after the last pipe
	       if(fork()==0)
	       {
		    dup2(fd[i-1][0],0);				//taking input from the previous pipe
		    if(opflag==1)				//checking the output flag
		    {
			 int fd1=open(arguements[opdir+1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			 dup2(fd1,1);				//redirecting output to fd
			 arguements[opdir]='\0';
			 opflag=0;
		    }
		    if(append_flag==1)
		    {
			 write(2,arguements[apdir+1],strlen(arguements[apdir+1]));
			 write(2,"\n",1);
			 int fd1=open(arguements[apdir+1],O_CREAT|O_APPEND|O_WRONLY);
			 dup2(fd1,1);
			 arguements[apdir]='\0';
			 append_flag=0;
		    }
		    for(k=0;k<=npipes;k++)
		    {
			 close(fd[k][0]);			//closing all the pipes
			 close(fd[k][1]);
		    }
		    execvp(arguements[pipe_position[npipes]+1],&arguements[pipe_position[npipes]+1]);	//executing the command
		    //    execvp(&arguements[pipe_position[pipecount]+1]);
	       }
	       for(k=0;k<=npipes;k++)
	       {
		    close(fd[k][0]);				//closing all the pipes
		    close(fd[k][1]);
	       }
	       int status;
	       if(background==0)
		    for(k=0;k<=npipes;k++)
			 wait(NULL);	       //parents's waiting
	       //     else
	       //	    for(k=0;k<npipes;k++)
	       //		 wait(NULL);
	       continue;
	  }
	  dup2(savein,0);
	  dup2(saveout,1);

	  //if both input and output redirection is there
	  if(append_flag==1)
	  {
	       int stdout_original=dup(1);
	       int stdin_original=dup(0);
	       write(2,arguements[apdir+1],strlen(arguements[apdir+1]));
	       write(2,"\n",1);
	       int fd1=open(arguements[apdir+1],O_CREAT|O_APPEND|O_WRONLY);
	       dup2(fd1,1);
	       arguements[apdir]='\0';
	       append_flag=0;
	       int fd0;
	       if(ipflag==1)
	       {
		    fd0=open(arguements[ipdir+1],O_RDONLY);
		    dup2(fd0,0);
		    arguements[ipdir]='\0';
		    ipflag=0;
	       }
	       execute_command(*arguements,arguements,1);
	       dup2(stdin_original,0);
	       dup2(stdout_original,1);
	       close(fd0);
	       close(fd1);
	       continue;
	  }
	  if(opflag==1 && ipflag==1)
	  {
	       printf("GONE\n");
	       int stdout_original=dup(1);
	       int stdin_original=dup(0);
	       int fd0=open(arguements[ipdir+1],O_RDONLY);
	       int fd1=open(arguements[opdir+1],O_CREAT | O_WRONLY,0777);
	       dup2(fd0,0);
	       dup2(fd1,1);
	       arguements[ipdir]='\0';
	       arguements[opdir]='\0';
	       execute_command(*arguements,arguements,1);
	       dup2(stdin_original,0);
	       dup2(stdout_original,1);
	       close(fd0);
	       close(fd1);
	       continue;
	  }

	  //if only output redirection is there
	  if(opflag==1)
	  {
	       output_redirection(arguements,n);
	       continue;
	  }


	  //if only input redirection is there
	  if(ipflag==1)
	  {
	       input_redirection(arguements,n);
	       continue;
	  }
	  int arg=0;
	  if(strcmp(arguements[0],"quit")==0)
	       return 0;
	  if(strcmp(arguements[0],"pinfo")==0)
	  {
	       if(n>2)
		    printf("Enter only one arguement at one time\n");
	       else
		    process_info(arguements,n);
	       continue;
	  }
	  if(strcmp(arguements[0],"fg")==0)
	  {
	       if(n>2)
		    printf("Enter only one arguement at one time\n");
	       else if(arguements[1]!='\0' && (atoi(arguements[1])-1)<=end)
	       {
		    kill(q[atoi(arguements[1])-1].pid,SIGCONT);
		    fore_id=q[atoi(arguements[1])-1].pid;
		    waitpid(q[atoi(arguements[1])-1].pid,&status,0);	//changing the waitpid 3rd arguement to 0 for parent to wait
		    delete_queue(q[atoi(arguements[1])-1].pid);		
	       }
	       continue;
	  }
	  if(strcmp(arguements[0],"overkill")==0)
	  {
	       for(i=end;i>=0;i--)
		    kill_job(i+1,9);					//calling killjob in a loop
	       continue;
	  }
	  if(strcmp(arguements[0],"kjob")==0)
	  {
	       if(n>3)
		    printf("Enter only two arguements at one time\n");
	       else if(arguements[1]!='\0' && arguements[2]!='\0')
		    kill_job(atoi(arguements[1]),atoi(arguements[2]));
	       continue;
	  }
	  if(strcmp(arguements[0],"jobs")==0)
	  {
	       for(i=0;i<=end;i++)
	       {
		    printf("[%d] ",i+1);
		    j=0;
		    while(strcmp(q[i].name[j],"\0")!=0)			//printing the number of background processes
		    {
			 printf("%s ",q[i].name[j]);
			 j++;
		    }
		    printf("[%d]\n",q[i].pid);
	       }
	       continue;
	  }
	  if(strcmp(arguements[0],"cd")==0)
	  {
	       if(arguements[1]=='\0' || strcmp(arguements[1],"~")==0 || strcmp(arguements[1],"~/")==0)
		    chdir(home);
	       else if(arguements[1]!='\0')
		    chdir(arguements[1]);
	       continue;
	  }
	  if((strcmp(arguements[n-1],"&"))==0)
	  {
	       arguements[n-1]='\0';
	       fore_id=-1;
	       execute_command(arguements[0],arguements,0);
	  }
	  else
	       execute_command(arguements[0],arguements,1);
	  arg=0;
     }
     return 0;
}

