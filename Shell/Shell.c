#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define HISTORY_SIZE 1000 /*Size of list all jobs*/
#define BUFFER_SIZE 1000 /*Size for increase*/
#define JOB_SIZE 1000 /*Size of list backround jobs*/

volatile pid_t pid = 1; /*Pid current process*/
volatile int exitstate = 0; /*State last process*/

void handler(int s)
{
  if (s == SIGTSTP)
  {
    if (pid) /*In father*/
	  if (pid != 1) /*Pid of son*/
	  {
	    kill(pid, SIGSTOP);
		exitstate = 1;
 	  }
  }
  else
    if (s == SIGINT)
	  if (!pid) /*Son in foremode*/
      {
		signal(SIGINT, SIG_DFL);
		kill(getpid(), SIGINT);
		signal(SIGINT, handler);
		exitstate = 2;
	  }
}

struct process /*Process structure*/
{
	int number_of_arg;
	char **arguments; /*arguments[0] == command*/
	char *input, *output;	
	int input_file_flag;
	int output_file_flag; /*2:write to the end, 1:rewrite*/
	int conveyer_flag;
	int background_flag;
	struct process *next;
};

struct job /*List of commands*/
{
	int background;
	struct process *command;
	int number_of_job;
	struct job *next;
};

void del_history(char **history)
{
  int i = 0;
  if (history != NULL)
  {
    while (history[i] != NULL)
    {
      free(history[i]);
      i++;
    }
    free(history);
  }
}

void del_command(struct process *command)
{
    int i = 0;
    struct process *tmp;
    while (command != NULL)
    {
      i = 0;
      if (command->arguments != NULL)
      {
        while (command->arguments[i] != NULL)
        {
          free(command->arguments[i]);
          i++;
        }
        free(command->arguments);
      }
      if (command->input != NULL)
      {
        free(command->input);
      }
      if (command->output != NULL)
      {
        free(command->output);
      }
      tmp = command->next;
      free(command);
      command = tmp;
    }
}

char *InputCommand(int argc, char **argv, char **history, int status)/*Input command and save without comments*/
{
  int buffer_size;
  int i = -1, j; /*counters*/
  char *buffer = NULL;
  char smvl, smvl2, smvl3;
  int flag = 0; /*end of string*/
/*$*/
  char *variable;
  int var_counter;
  int flag_variable;
  int numb_count;
  int var_number;
  char *arg; /*$#*/
  char *name; /*$USER, $HOME*/
  char path[255]; /*$PWD*/
/*!*/
  char *command;
  int com_counter;

  printf("%s$ ", getenv("USER"));
  buffer_size = BUFFER_SIZE;
  buffer = (char *) malloc (buffer_size * (sizeof(char)));
  if (buffer == NULL) /*Error memory of buffer*/
  {
    perror("Error of buffer allocation\n");
	free(buffer);
	return NULL;
  }
  while(1) /*Input symbol*/
  {
    if (!flag) /*Not end of string*/
      smvl = getchar(); /*INPUT*/

	if ((smvl == '\n') || (smvl == EOF)) /*It was input of the command. Continue programm*/
	{
      if (!flag)
	    i++;
	  if (i >= buffer_size) /*The command should have size lessen then buffer size, so increase buffer size*/
      {
        buffer_size = buffer_size + BUFFER_SIZE;
        buffer = realloc (buffer, buffer_size);
        if (buffer == NULL) /*Error of memory buffer*/
        {
          perror("\nError of memory buffer\n");
		  free(buffer);
	      return NULL;
        }
      }
	  buffer[i] = smvl;
	  return buffer;
	}
    if (smvl == '$') /*It is a variable*/
    {
	  variable = (char *) malloc ((sizeof(char)));
      if (variable == NULL)
      {
        perror("Error memory for variable\n");
		free(variable);
		free(buffer);
        return NULL;
      }
      var_counter = 0;
	  flag_variable = 0;
	  numb_count = 0;
	  if ((smvl = getchar()) == '{')
	  {
		flag_variable++;
		smvl = getchar();
	  }
      while ((smvl != EOF) && (smvl != '\n') && (smvl != '$') && (smvl != '<') && (smvl != '>') && (smvl != '\\') && (smvl != '&')
		  && (smvl != '!') && (smvl != '\"') && (smvl != '\'') && (smvl != ';') && (smvl != '{'))
	  {
	    if (smvl == '}')
		{
		  flag_variable--;
		  break;
		}
		if ((smvl == '#') && (var_counter))
		  break;
		variable = (char *) realloc (variable, (var_counter + 1) * (sizeof(char)));
		if (variable == NULL)
        {
          perror("Error memory for variable\n");
		  free(variable);
		  free(buffer);
          return NULL;
        }
	    variable[var_counter] = smvl;
	    var_counter++;
		if ((smvl >= '0') && (smvl <= '9'))
	      numb_count++;
		if (((var_counter == 2) && ((smvl == '#') || (smvl == '?'))) ||
	    ((var_counter == 4) && (((variable[0] == 'U') && (variable[1] == 'I') && (variable[2] == 'D')) ||
        ((variable[0] == 'P') && (variable[1] == 'I') && (variable[2] == 'D')) ||
		((variable[0] == 'P') && (variable[1] == 'W') && (variable[2] == 'D')))) ||
		((var_counter == 5) && (((variable[0] == 'H') && (variable[1] == 'O') && (variable[2] == 'M') && (variable[3] == 'E')) ||
        ((variable[0] == 'U') && (variable[1] == 'S') && (variable[2] == 'E') && (variable[3] == 'R')))))
		  break;
		smvl = getchar();
	  }
	  variable = (char *) realloc (variable, (var_counter + 1) * (sizeof(char)));
	  if (variable == NULL)
      {
        perror("Error memory for variable\n");
		free(variable);
		free(buffer);
        return NULL;
      }
	  variable[var_counter] = '\0';
	  if (!flag_variable)
	  {
        if (!strcmp(variable, "#")) /*Number of arguments of shell*/
	    {
	      j = 0;
		  var_number = argc - 1;
		  arg = (char *) malloc (sizeof(char));
		  if (arg == NULL)
          {
            perror("Error memory for variable\n");
		    free(variable);
			free(arg);
		    free(buffer);
            return NULL;
          }
		  if (!var_number)
		  {
			arg = (char *) realloc (arg, (j + 1) * sizeof(char));
			if (arg == NULL)
            {
              perror("Error memory for variable\n");
		      free(variable);
			  free(arg);
		      free(buffer);
              return NULL;
            }
            arg[j] = (char)((var_number % 10) + (int)'0');
			j++;
		  }
		  while (var_number) /*Create argc into string*/
	      {
		    arg = (char *) realloc (arg, (j + 1) * sizeof(char));
			if (arg == NULL)
            {
              perror("Error memory for variable\n");
		      free(variable);
			  free(arg);
		      free(buffer);
              return NULL;
            }
            arg[j] = (char)((var_number % 10) + (int)'0');
		    var_number = var_number / 10;
		    j++;
		  }
		  arg = (char *) realloc (arg, (j + 1) * sizeof(char));
		  if (arg == NULL)
          {
            perror("Error memory for variable\n");
		    free(variable);
	        free(arg);
		    free(buffer);
            return NULL;
          }
          arg[j] = '\0';
		  while (j) /*Write variable - argc*/
		  {
	        i++;
            if (i >= buffer_size) /*Increase buffer*/
	        {
		      buffer_size = buffer_size + BUFFER_SIZE;
		      buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		      if (buffer == NULL) /*Error of increase buffer*/
	          {
		        perror("Error of buffer allocation\n");
		        free(buffer);
			    free(variable);
			    free(arg);
			    return NULL;
		      }
	        }
	        buffer[i] = arg[j - 1]; /*Input symbol*/
		    j--;
		  }
		  free(arg);
	    }
	    else
	    {
		if (!strcmp(variable, "?")) /*status last command*/
		{
		  i++;
          if (i >= buffer_size) /*Increase buffer*/
	      {
		    buffer_size = buffer_size + BUFFER_SIZE;
		    buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		    if (buffer == NULL) /*Error of increase buffer*/
	        {
		      perror("Error of buffer allocation\n");
		      free(buffer);
			  free(variable);
			  return NULL;
		    }
	      }
		  if (status)
		  {
			if (status == 1)
			  buffer[i] = '1';
		    else buffer[i] = '2';
		  }
		  else
			buffer[i] = '0';
		}
		else
		{
        if ((!strcmp(variable, "USER")) || ((!strcmp(variable, "HOME")))) /*Name of user or home directory*/
	    {
	      name = getenv(variable);
		  j = 0;
		  while (name[j] != '\0')
		  {
	        i++;
            if (i >= buffer_size) /*Increase buffer*/
	        {
		      buffer_size = buffer_size + BUFFER_SIZE;
		      buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
		      if (buffer == NULL) /*Error of increase buffer*/
	          {
		        perror("Error of buffer allocation\n");
		        free(buffer);
				free(variable);
				return NULL;
		      }
	        }
	        buffer[i] = name[j]; /*Input symbol*/
		    j++;
		  }
	    }
	    else
	    {
	    if (!strcmp(variable, "SHELL")) /*Name of this program*/
	    {
		  j = 0;
		  while (argv[0][j] != '\0')
		  {
	        i++;
            if (i >= buffer_size) /*Increase buffer*/
	        {
		      buffer_size = buffer_size + BUFFER_SIZE;
		      buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		      if (buffer == NULL) /*Error of increase buffer*/
	          {
		        free(buffer);
			    free(variable);
			    return NULL;
		      }
	        }
	        buffer[i] = argv[0][j]; /*Input symbol*/
		    j++;
		  }
	    }
	    else
	    {
	    if ((!strcmp(variable, "UID")) || (!strcmp(variable, "PID"))) /*UID of user or PID of shell*/
	    {
	      j = 0;
		  if (!strcmp(variable, "UID"))
		    var_number = getuid();
	      else
		    var_number = getpid();
		  arg = (char *) malloc (sizeof(char));
		  if (arg == NULL) /*Error of increase buffer*/
	      {
		    perror("Error of buffer allocation\n");
		    free(buffer);
		    free(variable);
			free(arg);
		    return NULL;
		  }
	      if (!var_number)
		  {
		    arg = (char *) realloc (arg, (j + 1) * sizeof(char));
			if (arg == NULL) /*Error of increase buffer*/
	        {
		      perror("Error of buffer allocation\n");
		      free(buffer);
		      free(variable);
			  free(arg);
		      return NULL;
		    }
            arg[j] = '0';
		    j++;
		  }
		  else
          {
		    if (var_number < 0)
		    {
		      arg = (char *) realloc (arg, (j + 1) * sizeof(char));
			  if (arg == NULL) /*Error of increase buffer*/
	          {
		        perror("Error of buffer allocation\n");
		        free(buffer);
		        free(variable);
			    free(arg);
		        return NULL;
		      }
              arg[j] = '-';
			  j++;
			  var_number = -var_number;
		    }
		    while (var_number) /*Create argc into string*/
	        {
		      arg = (char *) realloc (arg, (j + 1) * sizeof(char));
			  if (arg == NULL) /*Error of increase buffer*/
	          {
		        perror("Error of buffer allocation\n");
		        free(buffer);
		        free(variable);
			    free(arg);
		        return NULL;
		      }
              arg[j] = (char)((var_number % 10) + (int)'0');
		      var_number = var_number / 10;
		      j++;
		    }
		  }
		  arg = (char *) realloc (arg, (j + 1) * sizeof(char));
		  if (arg == NULL) /*Error of increase buffer*/
	      {
		    perror("Error of buffer allocation\n");
		    free(buffer);
		    free(variable);
			free(arg);
		    return NULL;
		  }
          arg[j] = '\0';
		  while (j) /*Write variable - UID*/
		  {
	        i++;
            if (i >= buffer_size) /*Increase buffer*/
	        {
		      buffer_size = buffer_size + BUFFER_SIZE;
		      buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		      if (buffer == NULL) /*Error of increase buffer*/
	          {
		        perror("Error of buffer allocation\n");
		        free(buffer);
				free(variable);
				free(arg);
				return NULL;
		      }
	        }
	        buffer[i] = arg[j - 1]; /*Input symbol*/
		    j--;
	 	  }
		  free(arg);
	    }
	    else
	    {
	    if (!strcmp(variable, "PWD")) /*Current directory*/
	    {
		  if (getcwd(path, sizeof(path)) == NULL)
		  {
		    perror("pwd: Error of determine the way\n");
		    free(buffer);
			free(variable);
			return NULL;
	      }
          else
		  {
		    j = 0;
		    while (path[j] != '\0')
		    {
	          i++;
              if (i >= buffer_size) /*Increase buffer*/
	          {
		        buffer_size = buffer_size + BUFFER_SIZE;
		        buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		        if (buffer == NULL) /*Error of increase buffer*/
	            {
		          perror("Error of buffer allocation\n");
		          free(buffer);
				  free(variable);
				  free(arg);
				  return NULL;
		        }
	          }
	          buffer[i] = path[j]; /*Input symbol*/
		      j++;
		    }
		  }
	    }
        else
        {
	    if (var_counter == numb_count) /*Argument of shell*/
	    {
		  if ((atoi(variable) <= 0) || (atoi(variable) > argc))
		  {
		    perror("The requirement of a non-existent argument\n");
		  }
		  else
		  {
		  j = 0;
		  while (argv[atoi(variable) - 1][j] != '\0')
		  {
	        i++;
            if (i >= buffer_size) /*Increase buffer*/
	        {
		      buffer_size = buffer_size + BUFFER_SIZE;
		      buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		      if (buffer == NULL) /*Error of increase buffer*/
	          {
		        free(buffer);
			    free(variable);
			    return NULL;
		      }
	        }
	        buffer[i] = argv[atoi(variable) - 1][j]; /*Input symbol*/
		    j++;
		  }
		  }
	    }
	    else /*Other symbols - Just string with $*/
	    {
		  name = getenv(variable);
		  j = 0;
		  if (name != NULL)
		  {
		    while (name[j] != '\0')
		    {
	          i++;
              if (i >= buffer_size) /*Increase buffer*/
	          {
		        buffer_size = buffer_size + BUFFER_SIZE;
		        buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
		        if (buffer == NULL) /*Error of increase buffer*/
	            {
		          perror("Error of buffer allocation\n");
		          free(buffer);
				  free(variable);
				  return NULL;
		        }
	          }
	          buffer[i] = name[j]; /*Input symbol*/
		      j++;
		    }
	      }
		}
	    }
	    }
	    }
	    }
        }
        }
	  if (smvl == '}')
	    smvl = getchar();
      }
	  free(variable);
	  if(smvl == '}')
	    smvl = getchar();
	  if ((smvl == '\n') || (smvl == EOF)) /*It was input of the command. Continue programm*/
	  {
        if (!flag)
	      i++;
	    if (i >= buffer_size) /*The command should have size lessen then buffer size, so increase buffer size*/
        {
          buffer_size = buffer_size + BUFFER_SIZE;
          buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
          if (buffer == NULL) /*Error of memory buffer*/
          {
            perror("Error of memory buffer\n");
	        free(buffer);
			return NULL;
          }
        }
	    buffer[i] = smvl;
	    return buffer;
	  }
    }
	if (smvl == '\\') /*Shielding*/
	{
	  smvl = getchar();
	  if (smvl == EOF)
	  {
	    if (!flag)
	      i++;
	    if (i >= buffer_size) /*The command should have size lessen then buffer size, so increase buffer size*/
        {
          buffer_size = buffer_size + BUFFER_SIZE;
          buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
          if (buffer == NULL) /*Error of memory buffer*/
          {
            perror("Error of memory buffer\n");
			free(buffer);
	        return NULL;
          }
        }
	    buffer[i] = smvl;
	    return buffer;
	  }
	  if (smvl == '\n')
	  {
		printf("%s$ ", getenv("USER"));
		continue;
	  }
	  if ((smvl == '$') || (smvl == '<') || (smvl == '>') || (smvl == '#') || (smvl == '\\') || (smvl == '&')
		  || (smvl == '!') || (smvl == '\"') || (smvl == '\'') || (smvl == ';') || (smvl == '{') || (smvl == '}')) /*Usual symbols*/
	  {
	    i++;
        if (i >= buffer_size) /*Increase buffer*/
	    {
		  buffer_size = buffer_size + BUFFER_SIZE;
		  buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
		  if (buffer == NULL) /*Error of increase buffer*/
	      {
		    perror("Error of buffer allocation\n");
			free(buffer);
		    return NULL;
		  }
	    }
	    buffer[i] = '\\';
	    i++;
        if (i >= buffer_size) /*Increase buffer*/
	    {
		  buffer_size = buffer_size + BUFFER_SIZE;
		  buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
		  if (buffer == NULL) /*Error of increase buffer*/
	      {
		    perror("Error of buffer allocation\n");
			free(buffer);
		    return NULL;
		  }
	    }
	    buffer[i] = smvl;
		continue;
	  }
	  else /*Specific symbols*/
	  {
	    i++;
        if (i >= buffer_size) /*Increase buffer*/
	    {
		  buffer_size = buffer_size + BUFFER_SIZE;
		  buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
		  if (buffer == NULL) /*Error of increase buffer*/
	      {
		    perror("Error of buffer allocation\n");
			free(buffer);
		    return NULL;
		  }
	    }
	    buffer[i] = smvl;
		continue;
	  }
	}
    if (smvl == '\"')
	{
	  i++;
	  buffer[i] = smvl;
	  while(1)
	  {
	    i++;
		if (i >= buffer_size) /*Increase buffer*/
        {
          buffer_size = buffer_size + BUFFER_SIZE;
          buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
          if (buffer == NULL) /*Error of allocation*/
          {
            perror("Error of buffer allocation\n");
	        free(buffer);
			return NULL;
          }
        }
		smvl2 = getchar();
		buffer[i] = smvl2;
		if (smvl2 == '\"')
		  break;
		if ((smvl2 == EOF) || (smvl2 == '\n'))
		{
          buffer[i + 1] = '\0';
		  return buffer;
		}
       }
	}
	if (smvl == '\'')
	{
	  i++;
      buffer[i] = smvl;
	  while(1)
	  {
		i++;
		if (i >= buffer_size)
		{
		  buffer_size = buffer_size + BUFFER_SIZE;
		  buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		  if (buffer == NULL)
		  {
		    perror("Error buffer memory\n");
			free(buffer);
			return NULL;
		  }
		}
		smvl2 = getchar();
		buffer[i] = smvl2;
		if (smvl2 == '\'')
		  break;
		if ((smvl2 == EOF) || (smvl2 == '\n'))
		{
          buffer[i + 1] = '\0';
		  return buffer;
		}
	  }
	}
	if (smvl == '#') /*Ignore next symbols*/
	{
	  while(1)
	  {
		smvl = getchar();
		if ((smvl == EOF) || (smvl == '\n')) /*It was input of the command. Continue programm*/
		{	
		  i++;
          if (i >= buffer_size) /*The command should have size lessen then buffer size, so increase buffer size*/
          {
            buffer_size = buffer_size + BUFFER_SIZE;
            buffer = (char *) realloc (buffer, buffer_size * sizeof(char));
            if (buffer == NULL) /*Error of memory*/
            {
              perror("Error of buffer memory\n");
	          free(buffer);
			  return NULL;
            }
          }
		  buffer[i] = smvl;
		  buffer[i + 1] = '\0';
          return buffer;
		}
	  }
	}
    if (smvl == '!') /*Command with this number*/
    {
	  command = (char *) malloc (sizeof(char));
	  if (command == NULL)
	  {
		perror("The requirement of a non-existent command\n");
		free(command);
		free(buffer);
		return NULL;
      }  
	  com_counter = 0;
	  while (((smvl = getchar()) != EOF) && (smvl != '\n') && (smvl != '$') && (smvl != '<') && (smvl != '>') && (smvl != '\\') && (smvl != '&')
		  && (smvl != '!') && (smvl != '\"') && (smvl != '\'') && (smvl != ';'))
	  {
	    if ((smvl >= '0') && (smvl <= '9')) /*Argument of shell*/
	    {
		  command = (char *) realloc (command, (com_counter + 1) * (sizeof(char)));
		  if (command == NULL)
		  {
		    perror("The requirement of a non-existent command\n");
		    free(command);
		    free(buffer);
		    return NULL;
		  }
	      command[com_counter] = smvl;
		  com_counter++;
	    }
		else
		  break;
	  }
	  command = (char *) realloc (command, (com_counter + 1) * (sizeof(char)));
	  if (command == NULL)
	  {
		perror("The requirement of a non-existent command\n");
		free(command);
		free(buffer);
		return NULL;
      }
	  command[com_counter] = '\0';
	  if ((smvl == EOF) || (smvl == '\n'))
		flag = 1;
	  if (atoi(command) > 0)
	  {
	    j = 0;
	    while (history[j] != NULL)
	    {
		  if (j == (atoi(command) - 1))
		    break;
	      j++;
	    }
	    if (history[j] == NULL)
	    {
		  perror("The requirement of a non-existent command\n");
		  free(command);
		  continue;
        }
	    j = 0;
	    while (history[atoi(command) - 1][j] != '\0')
	    {
	      i++;
          if (i >= buffer_size) /*Increase buffer*/
	      {
		    buffer_size = buffer_size + BUFFER_SIZE;
		    buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		    if (buffer == NULL) /*Error of increase buffer*/
	        {
		      perror("Error of buffer allocation\n");
		      free(buffer);
			  free(command);
			  return NULL;
		    }
	      }
	      buffer[i] = history[atoi(command) - 1][j]; /*Input symbol*/
		  j++;
        }
	  }
	  else
	    perror("The requirement of a non-existent command\n");
	  free(command);
	  continue;
    }
	if ((smvl != '#') && (smvl != '\'') && (smvl != '\"') && (smvl != EOF) && (smvl != '\\') && (smvl != '$') && (smvl != '!') && (smvl != '\n')) /*Save other symbol*/
	{
	  i++;
      if (i >= buffer_size) /*Increase buffer*/
	  {
		buffer_size = buffer_size + BUFFER_SIZE;
		buffer = (char *) realloc(buffer, buffer_size * sizeof(char));
		if (buffer == NULL) /*Error of increase buffer*/
	    {
		  perror("Error of buffer allocation\n");
		  free(buffer);
		  return NULL;
		}
	  }
	  buffer[i] = smvl; /*Input symbol*/
	}
    else
      if ((smvl == '\n') || (smvl == EOF))
        flag = 1;
  }
  return buffer;
}

struct job *CreateList()
{
  struct job *list;
  list = NULL;
  list = (struct job *) malloc (sizeof(struct job));
  if (list == NULL)
  {
	perror("Error of creation of list\n");
	free(list);
	return NULL;
  }
  list->next = NULL;
  list->number_of_job = 0;
  list->command = NULL;
  list->background = 0;

  return list;
}

struct job* PushBack(struct job *head, struct process *command, int background)
{
  struct job *list = head, *tmp;

  if ((head == NULL) || (command == NULL))
  {
	perror("Error memory list");
    return NULL;
  }
  while (list->next != NULL)
	list = list->next;
  tmp = (struct job*) malloc (sizeof(struct job));
  if (tmp == NULL)
  {
    perror("Error memory job\n");
    return NULL;
  }
  tmp->number_of_job = list->number_of_job + 1;
  tmp->command = command;
  tmp->background = background;
  tmp->next = NULL;
  list->next = tmp;

  return list;
}

struct job *analyze(char *line, char **history, int argc, char **argv)
{
  struct job *job = CreateList();
  struct process *command = NULL;
  struct process *cmd = NULL;
  struct process *tmp = NULL;
  
  char *variable;
  char *var = NULL, *path = NULL;

  int var_counter;
  int conveyer_flag = 0;
  int background;
  int flag, flag2;
  int path_counter, path_buffer;
  int i, j = 0, k = 0;
  int arg_counter = 0, line_counter = 0, input_counter, output_counter;
  int buffer_size1 = BUFFER_SIZE, buffer_size2 = BUFFER_SIZE, input_buffer = BUFFER_SIZE, output_buffer = BUFFER_SIZE;
  int enter;
  int conveyer = 0;

  if (line == NULL) /*Error line*/
  {
    perror("Error line\n");
    return NULL;
  }

  while(1)
  {
    background = 0;
    while ((line[j] == ' ') || (line[j] == '\t')) /*Empty symbols*/
      j++;
    if (line[j] == '\n') /*Empty string*/
      break;

/*Write command*/
	command = (struct process *) malloc (sizeof(struct process));
    if (command == NULL) /*Error memory for command*/
    {
      perror("Error memory for command\n");
	  return NULL;
    }

	if (conveyer)
    {
      command->input_file_flag = 1;
	  conveyer = 0;
	}
    else
	  command->input_file_flag = 0;
    command->output_file_flag = 0;
    command->background_flag = 0;
    command->number_of_arg = 0;
	command->input = NULL;
	command->output = NULL;
	command->arguments = NULL;
	command->arguments = (char **) malloc (buffer_size1 * (sizeof(char*)));
	if (command->arguments == NULL) /*Error memomry for argumentsof command*/
    {
      perror("Error memomry for arguments of command\n");
	  del_command(command);
	  return NULL;
    }
	i = 0;
	while (i != buffer_size1) /*Rooms for arguments of command*/
    {
      command->arguments[i] = NULL;
	  i++;
	}
    cmd = command;
    arg_counter = 0;
	while ((line[j] != ';') && (line[j] != '\n') && (line[j] != EOF))
	{
      while ((line[j] == ' ') || (line[j] == '\t'))
        j++;
	  if (((command->background_flag) == 1) && (line[j] != ';') && (line[j] != '\n')) /*Wrong background parametrs*/
	  {
	    perror("Wrong background parametrs\n");
        command->arguments[arg_counter] = NULL;
        del_command(cmd);
		free(line);
        return NULL;
	  }
      if ((command->arguments[0] == NULL) && ((line[j] == EOF) || (line[j] == ';') || (line[j] == '\'')
		 || (line[j]=='\"') || (line[j] == '|') || (line[j] == '<') || (line[j] == '>') || (line[j] == '&')))	/*Command begin since unknown symbol*/
	  {
	    perror("False symbols\n");
        del_command(cmd);
		return NULL;
	  }
	  if (command->arguments[0] == NULL)
	  {
        buffer_size2 = BUFFER_SIZE;
		command->arguments[0] = (char *) malloc (buffer_size2 * (sizeof(char)));
        if (command->arguments[0] == NULL) /*Error memory of name of arguments*/
        {
          perror("Error memory of name of arguments\n");
          del_command(cmd);
		  return NULL;
        }
		line_counter = 0;
		while ((line[j] != EOF) && (line[j] != ';') && (line[j] != '\n') && (line[j] != ' ') && (line[j] != '\t') && (line[j] != '<') &&
		      (line[j] != '>') && (line[j] != '\'') && (line[j] != '\"') && (line[j] != '|') && (line[j] != '&')) /*Write arguments of the command in the list*/
	    {
		  command->arguments[0][line_counter] = line[j];
		  j++;
		  line_counter++;
		  if (line_counter >= buffer_size2) /*Increase buffer*/
		  {
		    buffer_size2 = buffer_size2 + BUFFER_SIZE;
	        command->arguments[0] = (char *) realloc (command->arguments, buffer_size2);
		    if (command->arguments[0] == NULL) /*Error memory of arguments of command*/
			{
		      perror("Error memory of arguments of command\n");
			  del_command(cmd);
		      return NULL;
			}
		  }
		}
        command->arguments[0][line_counter] = '\0';
        arg_counter++;
        command->number_of_arg++;
	  }
	  else /*Known argument*/
	  {
	    if (line[j] == '<') /*Input file*/
		{
		  if (command->input_file_flag) /*More one input file*/
		  {
		    perror("More one input file\n");
            del_command(cmd);
            return NULL;
		  }
		  command->input_file_flag = 1;
		  command->input = (char *) malloc (input_buffer * (sizeof(char)));
		  if (command->input == NULL) /*Error memory for input file*/
          {
            perror("Error memory for input file\n");
            del_command(cmd);
            return NULL;
          }
          j++;
          while ((line[j] == ' ') || (line[j] == '\t'))
            j++;
          input_counter = 0;
		  while ((line[j] != ' ') && (line[j] != '\t') && (line[j] != EOF) && (line[j] != ';') && (line[j] != '\n') &&
				(line[j] != '>') && (line[j] != '|') && (line[j] != '&') && (line[j] != '<')) /*Input input_file*/
		  {
		    command->input[input_counter] = line[j];
	        j++;
            input_counter++;
		    if (input_counter >= input_buffer) /*Increase buffer*/
			{
		      input_buffer = input_buffer + BUFFER_SIZE;
			  command->input = (char *) realloc(command->input, input_buffer * sizeof(char));
			  if (command->input == NULL) /*Error memory for input buffer*/
			  {
			    perror("Error memory for input buffer\n");
                del_command(cmd);
                return NULL;
			  }
		    }
		  }
          command->input[input_counter] = '\0';
        }
		else
		{
		if (line[j] == '>') /*Output file*/
		{
		  if (command->output_file_flag) /*Input more then one output file*/
		  {
            perror("Input more then one output file\n");
            del_command(cmd);
            return NULL;
		  }
		  command->output_file_flag = 1;
		  command->output = (char *) malloc (output_buffer * (sizeof(char)));
		  if (command->output == NULL)
          {
            perror("Error memory for output file\n");
            del_command(cmd);
            return NULL;
          }
          j++;
          if (line[j] == '>') /*Append*/
          {
			command->output_file_flag = 2;
            j++;
          }
          while ((line[j] == ' ') || (line[j] == '\t'))
            j++;
          output_counter = 0;
          while ((line[j] != ' ') && (line[j] != '\t') && (line[j] != EOF) && (line[j] != ';') &&
			    (line[j] != '\n') && (line[j] != '|') && (line[j] != '&') && (line[j] != '<') && (line[j] != '>')) /*Input output_file*/
		  {
		    command->output[output_counter] = line[j];
		    j++;
			output_counter++;
			if (output_counter >= output_buffer)
			{
			  output_buffer = output_buffer + BUFFER_SIZE;
			  command->output = (char *) realloc (command->output, output_buffer * sizeof(char));
			  if (command->output == NULL) /*Error memory for output file*/
			  {
			    perror("Error memory for output file\n");
			    del_command(cmd);
                return NULL;
			  }
			}
		  }
          command->output[output_counter] = '\0';
        }
		else
		{
		if ((line[j] == '\'') || (line[j] == '\"'))
        {
          if ((command->input_file_flag == 1) || (command->output_file_flag == 1)) /*Wrong rooms for arguments*/
          {
            perror("Wrong rooms for arguments\n");
            del_command(cmd);
            return NULL;;
          }
          buffer_size2 = BUFFER_SIZE;
          command->arguments[arg_counter] = (char *) malloc (buffer_size2 * (sizeof(char)));
          if (command->arguments[arg_counter] == NULL) /*Error memory for arguments*/
          {
            perror("Error memory for arguments\n");
            del_command(cmd);
            return NULL;
          }
          line_counter = 0;
          if (line[j] == '\'')
          {
            j++;
            while (line[j] != '\'')
            {
              if ((line[j] == '\n') || (line[j] == EOF)) /*Wrong symbols*/
              {
                perror("Wrong symbols\n");
                del_command(cmd);
                return NULL;
              }
              command->arguments[arg_counter][line_counter] = line[j];
              j++;
              line_counter++;
              if (line_counter >= buffer_size2)
              {
                buffer_size2 = buffer_size2 + BUFFER_SIZE;
                command->arguments[arg_counter] = (char *) realloc (command->arguments[arg_counter], buffer_size2 * sizeof(char));
                if (command->arguments[arg_counter] == NULL) /*Error buffer of arguments*/
                {
                  perror("Error buffer of arguments\n");
                  del_command(cmd);
                  return NULL;
                }
              }
            }
            command->arguments[arg_counter][line_counter] = '\0';
          }
          else
          {
		    j++;
            while (line[j] != '\"')
            {	
			  flag = 0;
	          flag2 = 0;
              if ((line[j] == EOF) || (line[j] == '\n')) /*Wrong symbols*/
              {
                perror("Wrong symbols\n");
                del_command(cmd);
                return NULL;
              }
			  if (line[j] == '\\')
			  {
			    flag2 = 1;
		        j++;
				if ((line[j] == EOF) || (line[j] == '\n')) /*Wrong symbols*/
				{
				  perror("Wrong symbols\n");
                  del_command(cmd);
				  return NULL;
				}
			  }
			  command->arguments[arg_counter][line_counter] = line[j];
              j++;
              line_counter++;
              if (line_counter >= buffer_size2)
              {
                buffer_size2 = buffer_size2 + BUFFER_SIZE;       
                command->arguments[arg_counter] = (char *) realloc (command->arguments[arg_counter], buffer_size2 * sizeof(char));
                if (command->arguments[arg_counter] == NULL)
                {
                  perror("Error memory in arguments\n");
                  del_command(cmd);
				  return NULL;
                }
              }
            }
          }
          command->arguments[arg_counter][line_counter] = '\0';
          arg_counter++;
          command->number_of_arg++;
          if (arg_counter >= buffer_size1)
          {
            buffer_size1 = buffer_size1 + BUFFER_SIZE;
            command->arguments = (char **) realloc (command->arguments, buffer_size1 * sizeof(char *));
            if (command->arguments == NULL) /*Error memory for arguments*/
            {
              perror("Error memory for arguments\n");
              del_command(cmd);
		      return NULL;
            }
		    while (i != buffer_size1)
		    {
		      command->arguments[i] = NULL;
			  i++;
		    }
		  }
          j++;
        }
        else
        {
		if (line[j] == '&') /*Background mode*/
		{
		  if (command->background_flag)
	  	  {
		    perror("Double background\n");
            del_command(cmd);
            return NULL;
		  }
		  else
		  {
			command->background_flag = 1;
			background = 1;
			k = j + 1;
			while ((line[k] == ' ') || (line[k] == '\t'))
              k++;
			if ((line[k] != ';') || (line[k] != EOF) || (line[k] != '\n') || (line[k] != '|'))
			  break;
		    else
			  j = k;
		  }
		}
		else
		{
		if (line[j] == '|') /*Conveyer*/
		{
		  if (command->output_file_flag) /*Input more then one output file*/
		  {
            perror("Input more then one output file\n");
            del_command(cmd);
            return NULL;
		  }
		  command->output_file_flag = 1;
		  conveyer = 1;
		  break;
        }
        else
        {
        if ((line[j] != ';') && (line[j] != EOF) && (line[j] != '\n')) /*End of input command*/
        {
		  buffer_size2 = BUFFER_SIZE;
          command->arguments[arg_counter] = (char *) malloc (buffer_size2 * (sizeof(char)));
          if (command->arguments[arg_counter] == NULL) /*Error room for argument*/
          {
            perror("Error room for argument\n");
            del_command(cmd);
		    return NULL;
          }
          line_counter = 0;
          while ((line[j] != ' ') && (line[j] != ';') && (line[j] != '\n') && (line[j] != EOF) && (line[j] != '\t') && (line[j] != '|') &&
                (line[j] != '>') && (line[j] != '<') && (line[j] != '&') && (line[j] != '\'') && (line[j] != '\"'))
          {
            command->arguments[arg_counter][line_counter] = line[j];
            j++;
            line_counter++;
            if (line_counter >= buffer_size2)
            {
              buffer_size2 = buffer_size2 + BUFFER_SIZE;
              command->arguments[arg_counter] = (char *) realloc(command->arguments, buffer_size2 * sizeof(char));
              if (command->arguments[arg_counter] == NULL) /*Error memory for arguments*/
              {
                perror("Error memory for arguments\n");
                del_command(cmd);
				return NULL;
              }
            }
          }
          command->arguments[arg_counter][line_counter] = '\0';  
          command->number_of_arg++;
          arg_counter++;
          if (arg_counter >= buffer_size1)
          {
            buffer_size1 = buffer_size1 + BUFFER_SIZE;
            command->arguments = (char **) realloc (command->arguments, buffer_size1 * sizeof(char *));
            if (command->arguments == NULL) /*Error memory for arguments*/
            {
              perror("Error memory for arguments\n");
              del_command(cmd);
			  return NULL;
            }
          }   
        }
        }
        }
	    }
	    }
        }
	  }
    }

	if ((line[j] == EOF) || (line[j] == '\n')) /*End of input commands*/
	{
	  command->arguments[arg_counter] = NULL;
	  PushBack(job, cmd, background);
	  if (conveyer)
	  {
	    perror("Error of the end of conveyer");
		del_command(cmd);
        return NULL;
	  }
	  break;
	}
	if ((line[j] == ';') || (line[j] == '|') || (line[j] == '&')) /*End of input this command*/
	{
	  command->arguments[arg_counter] = NULL;
	  PushBack(job, cmd, background);
      j++;
    }
  }

  return job;	
}


int main(int argc, char **argv)
{
  int i = -1, j, k, l, t = -1; /*counters*/
  int buffer_size = BUFFER_SIZE;
  int buffer_size_job = BUFFER_SIZE;
  char **history;
  char **mas_jobs;
  pid_t *mas_pid;
  int *state;
  char *line; /*new command*/
  struct job *job;
  char path[255];
  char *conveyer;
  
  int flag;
  int inputfile, outputfile, input, output;
  int conv_last = 0, conv_next = 0;
  int channel[2], channel2[2];
  char smvl;

/*Clean screen*/
 /* if ((pid = fork()) == -1)
  {
    perror("Error of crearion of son - that clear screen");
    return 1;
  }*/
 // if (!pid) /*Son - clean screen*/
  /*{
    execlp("clear", "clear", (char *)0);
    perror("Error of process clear screen");
    return 1;
  }
  waitpid(pid, NULL, 0);*/
  pid = 1;
  signal(SIGTSTP, handler);
  signal(SIGINT, handler);
/*Create history and work with commands*/
  history = (char **) malloc (HISTORY_SIZE * sizeof(char *));/*Rooms for 1000 possible commands*/
  mas_jobs = (char **) malloc (JOB_SIZE * sizeof(char *));
  mas_pid = (pid_t *) malloc (JOB_SIZE * sizeof(pid_t));
  state = (int *) malloc (JOB_SIZE * sizeof(int));
  while(1)
  {
    line = InputCommand(argc, argv, history, exitstate); /*Same name of new command*/
	j = 0;
    k = 0;
	while (j <= t) /*Check on the end of job_command*/
	{
	  k++;
      if ((!state[j]) || ((state[j] != -1) && (waitpid(mas_pid[j], NULL, WNOHANG)))) /*Process finished*/
	  {
		printf("[%4d  ] Stopped     ",  k);
	    l = 0;
        while (mas_jobs[j][l] != '\n')
        {
          printf("%c",mas_jobs[j][l]);
          l++;
        }
		printf("\n");  
	    free(mas_jobs[j]);
		l = j;
        while (l != t)
        {
          mas_jobs[l] = mas_jobs[l + 1];
		  mas_pid[l] = mas_pid[l + 1];
	      state[l] = state[l + 1];
          l++;
        }
        mas_jobs[t] = NULL;
	    t--;
	  }
	  else
	    j++;
	}
/*Input in the history*/
    if (i != (HISTORY_SIZE - 1)) /*If there are rooms in the history*/
    {
	  i++;
      history[i] = (char *) malloc (buffer_size * (sizeof(char))); /*Memory for the new command*/
	  if (history[i] == NULL) /*Error of memory for new command*/
      {
		for (j = 0; j <= t; j++)
		   if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		   kill(mas_pid[j], SIGSTOP);
		del_history(history);
		free(line);
		del_history(mas_jobs);
		free(mas_pid);
		free(state);
        perror("Error of memory for new command\n");
	    return 1;
      }
      history[i + 1] = NULL; /*Next elements of history is empty*/
    }
    else /*If there are not rooms in the history*/
    {
      free(history[j]); /*Delete the first command*/
      while (j != (HISTORY_SIZE - 1)) /*Move all commands*/
      {
        history[j] = history[j + 1];
        j++;
      }
      history[j] = NULL; /*Next elements of history is empty*/
      history[i] = (char *) malloc (buffer_size * (sizeof(char)));
	  if (history[i] == NULL) /*Error of memory for new command*/
      {
		for (l = 0; l <= t; l++)
		  if (!waitpid(mas_pid[l], NULL, WNOHANG))
	 		kill(mas_pid[l], SIGSTOP);
		del_history(history);
		free(line);
		del_history(mas_jobs);
		free(mas_pid);
		free(state);
        perror("Error of memory for new command\n");
	    return 1;
      }
	}
    if (line != NULL) /*Input the command in history*/
    {
	  j = 0;
      while ((line[j] != '\n') && (line[j] != EOF)) /*Input the command in the element of history*/
      {
	    if (line[j] == EOF) /*Exit*/
		{
		  for (l = 0; l <= t; l++)
		    if (!waitpid(mas_pid[l], NULL, WNOHANG))
	 		    kill(mas_pid[l], SIGSTOP);
		  del_history(history);
		  free(line);
		  del_history(mas_jobs);
		  free(mas_pid);
		  free(state);
		  return 0;
		}
        history[i][j] = line[j];
        j++;
        if (j >= buffer_size) /*Increase size of one string in the history*/
        {
          buffer_size = buffer_size + BUFFER_SIZE;
          history[i] = (char *) realloc (history[i], buffer_size * sizeof(char));
        }
      }
      history[i][j] = '\n'; /*Last symbol every command*/
    }

/*Determine the job*/
	job = analyze(line, history, argc, argv); /*Analize action after operate this command*/
    free(line);

/*Execute commands*/
    while ((job->next) != NULL)
    {
	  input = dup(0);
      output = dup(1);
      if (((job->next)->command)->input != NULL) /*Connect input with the input file*/
      {
		conv_next = 0;
        if ((inputfile = open(((job->next)->command)->input, O_RDONLY, 0666)) == -1) /*Not connect input in channel with the file*/
        {
          perror("Error: input file had been unfounded\n");
          del_history(history);
		  for (j = 0; j <= t; j++)
		    if (!waitpid(mas_pid[j], NULL, WNOHANG))
			  kill(mas_pid[j], SIGSTOP);
		  del_history(mas_jobs);
		  free(mas_pid);
		  free(state);
		  close(inputfile);
          close(outputfile);
          if (conv_next == 1)
	        close(channel[0]);
	      if (conv_next == 2)
	        close(channel2[0]);
	      dup2(input, 0);
          dup2(output, 1);
          close(input);
          close(output);
	      return 0;
        }
	    dup2(inputfile, 0);
      }
      else
		if (((job->next)->command)->input_file_flag) /*Next element of conveyer*/
		{
		  if (conv_last == 2) /*0-1-0*/
		  {
		    close(channel2[1]);
		    dup2(channel2[0], 0);
	      }
	      else /*1-0*/
	      {
		    close(channel[1]);
		    dup2(channel[0], 0);
	      }
	    }

      if (((job->next)->command)->output != NULL) /*Connect output with the file*/
      {
		conv_last = 0;
        if (((job->next)->command)->output_file_flag == 1) /*Rewrite output file*/
          outputfile = open(((job->next)->command)->output, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        else /*Append output file*/
          outputfile = open(((job->next)->command)->output, O_CREAT | O_WRONLY | O_APPEND, 0666);

		if (outputfile == -1) /*Not connect output in channel with the file*/
        {
          perror("Error: output file had been unfounded\n");
		  del_history(history);
		  for (j = 0; j <= t; j++)
		    if (!waitpid(mas_pid[j], NULL, WNOHANG))
			  kill(mas_pid[j], SIGSTOP);
		  del_history(mas_jobs);
	  	  free(mas_pid);
		  free(state);
		  close(inputfile);
          close(outputfile);
          if (conv_next == 1)
	        close(channel[0]);
	      if (conv_next == 2)
	        close(channel2[0]);
	      dup2(input, 0);
          dup2(output, 1);
          close(input);
          close(output);
	      return 0;
        }
        dup2(outputfile, 1);
      }
      else
	    if (((job->next)->command)->output_file_flag) /*Last element of conveyer*/
	    {
		  conv_next = conv_last;
		  if (conv_next % 2) /*0-1*/
		  {
		    if (pipe(channel2)) /*Error of create channel*/
            {
              perror("Error to create of channel fd1");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
              del_history(history);
		      del_history(mas_jobs);
       	      free(mas_pid);
		      free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
	          return 0;
            }
		    dup2(channel2[1], 1);
		    conv_last = 2;
		  }
		  else /*0-0-1*/
		  {
		    if (pipe(channel)) /*Error of creation of channel*/
            {
              perror("Error to create of channel fd1");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
              del_history(history);
		      del_history(mas_jobs);
       	      free(mas_pid);
		      free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
	          return 0;
            }
			dup2(channel[1], 1);
		    conv_last = 1;
	      }
	    }
	    else
	    {
		  conv_next = conv_last;
	      conv_last = 0;
	    }

	  if (!strcmp((((job->next)->command)->arguments)[0], "exit")) /*exit*/
      {
		del_history(history);
		for (j = 0; j <= t; j++)
		  if (!waitpid(mas_pid[j], NULL, WNOHANG))
			kill(mas_pid[j], SIGSTOP);
		del_history(mas_jobs);
		free(mas_pid);
		free(state);
		if (((job->next)->command)->input != NULL)
	      close(inputfile);
	    if (((job->next)->command)->output != NULL)
          close(outputfile);
        if (conv_next == 1)
	      close(channel[0]);
	    if (conv_next == 2)
	      close(channel2[0]);
	    dup2(input, 0);
        dup2(output, 1);
        close(input);
        close(output);
	    return 0;
	  }
      else
      {
	  if (!strcmp((((job->next)->command)->arguments)[0], "cd")) /*cd*/
      {
		exitstate = 0;
		if (conv_next) /*Rewrite without last symbol ENTER*/
	    {
		  j = 0;
		  line = (char *) malloc (sizeof(char));
		  if (line == NULL)
		  {
			perror("Error memory for new parametr\n");
			for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			del_history(history);
			del_history(mas_jobs);
       		free(mas_pid);
			free(state);
		    close(inputfile);
            close(outputfile);
            if (conv_next == 1)
	          close(channel[0]);
	        if (conv_next == 2)
	          close(channel2[0]);
		    dup2(input, 0);
            dup2(output, 1);
            close(input);
            close(output);
	        return 0;
		  }
		  if (conv_next == 1)
		  {
		    while (read(channel[0], &smvl, sizeof(char)) == sizeof(char))
		    {
			  if (smvl == '\n')
			    if (read(channel[0], &smvl, sizeof(char)) == sizeof(char))
			    {
				  line = (char *) realloc (line, (j + 1) * sizeof(char));
				  if (line == NULL)
		          {
			        perror("Error memory for new parametr\n");
					for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
		       	    del_history(history);
			        del_history(mas_jobs);
       		        free(mas_pid);
			        free(state);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
				    dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 0;
		          }
			      line[j] = '\n';
			      j++;
			    }
			    else
			      break;
			  line = (char *) realloc (line, (j + 1) * sizeof(char));
			  if (line == NULL)
		      {
			    perror("Error memory for new parametr\n");
				for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			    del_history(history);
			    del_history(mas_jobs);
       		    free(mas_pid);
			    free(state);
		        close(inputfile);
                close(outputfile);
                if (conv_next == 1)
	              close(channel[0]);
	            if (conv_next == 2)
	              close(channel2[0]);
			    dup2(input, 0);
                dup2(output, 1);
                close(input);
                close(output);
	            return 0;
		      }
			  line[j] = smvl;
			  j++;
	        }
			line = (char *) realloc (line, (j + 1) * sizeof(char));
		    if (line == NULL)
		    {
			  perror("Error memory for new parametr\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			  del_history(history);
			  del_history(mas_jobs);
       		  free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
			  dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 0;
		    }
		    line[j] = '\0';
	      }
	      if (conv_next == 2)
	      {
	        while (read(channel2[0], &smvl, sizeof(char)) == sizeof(char))
		    {
			  if (smvl == '\n')
			    if (read(channel2[0], &smvl, sizeof(char)) == sizeof(char))
			    {
				  line = (char *) realloc (line, (j + 1) * sizeof(char));
				  if (line == NULL)
		          {
			        perror("Error memory for new parametr\n");
					for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			        del_history(history);
			        del_history(mas_jobs);
       		        free(mas_pid);
			        free(state);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
				    dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 0;
		          }
			      line[j] = '\n';
			      j++;
			    }
			    else
			      break;
			  line = (char *) realloc (line, (j + 1) * sizeof(char));
			  if (line == NULL)
		      {
			    perror("Error memory for new parametr\n");
				for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			    del_history(history);
			    del_history(mas_jobs);
       		    free(mas_pid);
			    free(state);
		        close(inputfile);
                close(outputfile);
                if (conv_next == 1)
	              close(channel[0]);
	            if (conv_next == 2)
	              close(channel2[0]);
			    dup2(input, 0);
                dup2(output, 1);
                close(input);
                close(output);
	            return 0;
		      }
			  line[j] = smvl;
			  j++;
	        }
	      }
		  line = (char *) realloc (line, (j + 1) * sizeof(char));
		  if (line == NULL)
		  {
			perror("Error memory for new parametr\n");
			for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
			del_history(history);
			del_history(mas_jobs);
       		free(mas_pid);
			free(state);
		    close(inputfile);
            close(outputfile);
            if (conv_next == 1)
	          close(channel[0]);
	        if (conv_next == 2)
	          close(channel2[0]);
		    dup2(input, 0);
            dup2(output, 1);
            close(input);
            close(output);
	        return 0;
		  }
		  line[j] = '\0';
		}
		if (((((job->next)->command)->number_of_arg) == 2) || ((conv_next) && (j) && ((((job->next)->command)->number_of_arg) == 1)))
        {
          if ((((((job->next)->command)->number_of_arg) == 2) && ((!strcmp((((job->next)->command)->arguments)[1], "~")) || (!strcmp((((job->next)->command)->arguments)[1], "/root"))))
		  || ((conv_next) && ((!strcmp(line, "~")) || (!strcmp(line, "/root")))))
          {
            if (chdir((getenv("HOME"))))
			  printf("cd: Error of transsmition to home directory\n");
          }
          else
          {
          if ((((((job->next)->command)->number_of_arg) == 2) && (!strcmp((((job->next)->command)->arguments)[1], "/")))
		  || ((conv_next) && (!strcmp(line, "/"))))
          {
            if (chdir("/"))
			  printf("cd: Error of transsmition to /\n");
          }
          else
          {
          if ((((((job->next)->command)->number_of_arg) == 2) && (!strcmp((((job->next)->command)->arguments)[1], "/home")))
		  || ((conv_next) && (!strcmp(line, "/home"))))
          {
            if (chdir("/home"))
			  printf("cd: Error of transsmition to /home\n");
          }
          else
          {
	      if (((((job->next)->command)->number_of_arg) == 2) && (!strcmp((((job->next)->command)->arguments)[1], ".."))
		  || ((conv_next) && (!strcmp(line, ".."))))
          {
            if (chdir(".."))
			  printf("cd: Error of transsmition to /home\n");
          }
          else
          {
			if (getcwd(path, sizeof(path)) != NULL)
			{
		      strcat(path, "/");
			  if (conv_next)
			  {
			    if (chdir(strcat(path, line)))
				 printf("cd: %s: No such file or directory\n", line);
			  }
			  else
			    if (chdir(strcat(path, job->next->command->arguments[1])))
				   printf("cd: %s: No such file or directory\n", job->next->command->arguments[1]);
		    }
			else
			  printf("cd: Error of determine the way\n");
          }
          }
          }
          }
        }
        else
		{
		  if (((((job->next)->command)->number_of_arg) > 2) || (((((job->next)->command)->number_of_arg) > 1) && (conv_next) && (j)))
		    printf("cd: So much files or directories\n");
		  else
            if (chdir((getenv("HOME"))))
			  printf("cd: Error of transsmition to home directory\n");
		}
        if (conv_next)
		  free(line);
	  }
	  else
	  {
	  if (!strcmp((((job->next)->command)->arguments)[0], "pwd")) /*pwd*/
      {
		exitstate = 0;
        if (getcwd(path, sizeof(path)) == NULL)
		  printf("pwd: Error of determine the way\n");
        else
		  printf("%s\n", path);
	  }
	  else
	  {
	  if (!strcmp((((job->next)->command)->arguments)[0], "history")) /*history*/
      {
		exitstate = 0;
        j = 0;
        while (j <= i)
        {
          if (history[j] != NULL)
          {
			printf("%4d  ", j + 1);
            k = 0;
            while (history[j][k] != '\n')
            {
              printf("%c", history[j][k]);
              k++;
            }
			printf("\n");
            j++;
          }
        }
      }
	  else
	  {
	  if (!strcmp((((job->next)->command)->arguments)[0], "jobs")) /*jobs*/
      {
		exitstate = 0;
		j = 0;
		l = 0;
        while (j <= t)
		  if (state[j] != 2) /*Not in the FM*/
		  {
			l++;
            printf("[%4d  ]  ", l);
		    if (state[j] == 1) /*BM*/
			  printf("Running  ");
			else /*Returned in FM*/
			  printf("Stopped  ");
            k = 0;
            while (mas_jobs[j][k] != '\n')
            {
              printf("%c",mas_jobs[j][k]);
              k++;
            }
            j++;
		    printf("\n");
		  }
      }
	  else
	  {
	  if (!strcmp((((job->next)->command)->arguments)[0], "fg")) /*fg*/
      {
		exitstate = 0;
		if (conv_next) /*Rewrite without last symbol ENTER*/
	    {
		  j = 0;
		  line = (char *) malloc (sizeof(char));
		  if (line == NULL)
		  {
			perror("Error memory for new parametr\n");
			for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	    del_history(history);
			del_history(mas_jobs);
       		free(mas_pid);
			free(state);
		    close(inputfile);
            close(outputfile);
            if (conv_next == 1)
	          close(channel[0]);
	        if (conv_next == 2)
	          close(channel2[0]);
		    dup2(input, 0);
            dup2(output, 1);
            close(input);
            close(output);
	        return 0;
		  }
		  if (conv_next == 1)
		  {
		    while (read(channel[0], &smvl, sizeof(char)) == sizeof(char))
		    {
			  if (smvl == '\n')
			    if (read(channel[0], &smvl, sizeof(char)) == sizeof(char))
			    {
				  line = (char *) realloc (line, (j + 1) * sizeof(char));
				  if (line == NULL)
		          {
			        perror("Error memory for new parametr\n");
					for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	            del_history(history);
			        del_history(mas_jobs);
       		        free(mas_pid);
			        free(state);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
				    dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 0;
		          }
			      line[j] = '\n';
			      j++;
			    }
			    else
			      break;
			  line = (char *) realloc (line, (j + 1) * sizeof(char));
			  if (line == NULL)
		      {
			    perror("Error memory for new parametr\n");
				for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	        del_history(history);
			    del_history(mas_jobs);
       		    free(mas_pid);
			    free(state);
		        close(inputfile);
                close(outputfile);
                if (conv_next == 1)
	              close(channel[0]);
	            if (conv_next == 2)
	              close(channel2[0]);
			    dup2(input, 0);
                dup2(output, 1);
                close(input);
                close(output);
	            return 0;
		      }
			  line[j] = smvl;
			  j++;
	        }
			line = (char *) realloc (line, (j + 1) * sizeof(char));
		    if (line == NULL)
		    {
			  perror("Error memory for new parametr\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       		  free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
			  dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 0;
		    }
		    line[j] = '\0';
	      }
	      if (conv_next == 2)
	      {
	        while (read(channel2[0], &smvl, sizeof(char)) == sizeof(char))
		    {
			  if (smvl == '\n')
			    if (read(channel2[0], &smvl, sizeof(char)) == sizeof(char))
			    {
				  line = (char *) realloc (line, (j + 1) * sizeof(char));
				  if (line == NULL)
		          {
			        perror("Error memory for new parametr\n");
					for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	            del_history(history);
			        del_history(mas_jobs);
       		        free(mas_pid);
			        free(state);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
				    dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 0;
		          }
			      line[j] = '\n';
			      j++;
			    }
			    else
			      break;
			  line = (char *) realloc (line, (j + 1) * sizeof(char));
			  if (line == NULL)
		      {
			    perror("Error memory for new parametr\n");
				for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	        del_history(history);
			    del_history(mas_jobs);
       		    free(mas_pid);
			    free(state);
		        close(inputfile);
                close(outputfile);
                if (conv_next == 1)
	              close(channel[0]);
	            if (conv_next == 2)
	              close(channel2[0]);
			    dup2(input, 0);
                dup2(output, 1);
                close(input);
                close(output);
	            return 0;
		      }
			  line[j] = smvl;
			  j++;
	        }
	      }
		  line = (char *) realloc (line, (j + 1) * sizeof(char));
		  if (line == NULL)
		  {
			perror("Error memory for new parametr\n");
			for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	    del_history(history);
			del_history(mas_jobs);
       		free(mas_pid);
			free(state);
		    close(inputfile);
            close(outputfile);
            if (conv_next == 1)
	          close(channel[0]);
	        if (conv_next == 2)
	          close(channel2[0]);
		    dup2(input, 0);
            dup2(output, 1);
            close(input);
            close(output);
	        return 0;
		  }
		  line[j] = '\0';
		}
		if (t != -1)
		{
        if (((((job->next)->command)->number_of_arg) == 1) || ((conv_next) && (!j)))/*Last background programm*/
        {
		  j = t;
		  while ((j) && (state[j] == 2))
			j--;
		  if (state[j] == 2) /*All command in FM*/
			printf("No command in background mode\n");
		  else
			{
			  if (state[j] == -1)
			  {
				kill(mas_pid[j], SIGSTOP);
				waitpid(pid, NULL, 0);
				printf("[%4d  ] Stopped     ",  k);
	            l = 0;
                while (mas_jobs[j][l] != '\n')
                {
                  printf("%c",mas_jobs[j][l]);
                  l++;
                }
		        printf("\n");  
			  }
			  else
			  {
			  pid = mas_pid[j];
              waitpid(pid, NULL, WUNTRACED);
			  if (exitstate == 1) /*ctrl+z*/
		      {
			    printf("[%4d  ] Stopped     ",  k);
	            l = 0;
                while (mas_jobs[j][l] != '\n')
                {
                  printf("%c",mas_jobs[j][l]);
                  l++;
                }
		        printf("\n");	
			    if (t != (JOB_SIZE - 1)) /*If there are rooms in the history*/
                {
	              t++;
                  mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char))); /*Memory for the new command*/
	              if (mas_jobs[t] == NULL) /*Error of memory for new command*/
                  {
                    perror("Error of memory for new command\n");
					for (j = 0; j <= t; j++)
		              if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		            kill(mas_pid[j], SIGSTOP);
	  	            del_history(history);
			        del_history(mas_jobs);
       	        	free(mas_pid);
			        free(state);
				    if (conv_next)
					  free(line);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
		            dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 1;
                  }
                  mas_jobs[t + 1] = NULL; /*Next elements of history is empty*/
                }
                else /*If there are not rooms in the history*/
                {
			      l = 0;
                  free(mas_jobs[j]); /*Delete the first command*/
                  while (l != (JOB_SIZE - 1)) /*Move all commands*/
                  {
                    mas_jobs[l] = mas_jobs[l + 1];
                    l++;
                  }
                  mas_jobs[l] = NULL; /*Next elements of history is empty*/
                  mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char)));
	              if (mas_jobs[t] == NULL) /*Error of memory for new command*/
                  {
                    perror("Error of memory for new command\n");
					for (j = 0; j <= t; j++)
		              if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		            kill(mas_pid[j], SIGSTOP);
	  	            del_history(history);
			        del_history(mas_jobs);
       	        	free(mas_pid);
			        free(state);
				    if (conv_next)
					  free(line);
		            close(inputfile);
                    close(outputfile);
                    if (conv_next == 1)
	                  close(channel[0]);
	                if (conv_next == 2)
	                  close(channel2[0]);
		            dup2(input, 0);
                    dup2(output, 1);
                    close(input);
                    close(output);
	                return 1;
                  }
	            }
                l = 0;
                while ((mas_jobs[j][l] != '\n') && (mas_jobs[j][l] != ';')  && (mas_jobs[j][l] != '\0')) /*Input the command in the element of history*/
                {
                  mas_jobs[t][l] = mas_jobs[j][l];
                  l++;
                  if (l >= buffer_size_job) /*Increase size of one string in the history*/
                  {
                    buffer_size_job = buffer_size_job + BUFFER_SIZE;
                    mas_jobs[t] = (char *) realloc (mas_jobs[t], buffer_size_job * sizeof(char));
                  }
		        }
                mas_jobs[t][l] = '\n'; /*Last symbol every command*/
		        mas_pid = (pid_t *) realloc (mas_pid, (t + 1) * sizeof(pid_t));
		        mas_pid[t] = mas_pid[j];
			    state = (pid_t *) realloc (state, (t + 1) * sizeof(pid_t));
		        state[t] = -1;
		      }
		      }
			  pid = 1;
			  free(mas_jobs[j]);
		      l = j;
              while (l != t)
              {
                mas_jobs[l] = mas_jobs[l + 1];
			    mas_pid[l] = mas_pid[l + 1];
			    state[l] = state[l + 1];
                l++;
              }
              mas_jobs[t] = NULL;
	          t--;
			}
		}
	    else /*Certain in the BM*/
	    {
		  if (((((job->next)->command)->number_of_arg) == 2) || (((((job->next)->command)->number_of_arg) == 1) && (conv_next) && (j)))
          {
			if (!conv_next)
			  line = (((job->next)->command)->arguments)[1];
		    k = 0;
	        while ((line[k] >= '0') && (line[k] <= '9'))
	   	      k++;
	        if ((line[k] == '\0') || (line[k] == '\n') || (line[k] == ';'))
		    {
	          if ((atoi(line) <= 0) || (atoi(line) > (t + 1)))
		        printf("The requirement of a non-existent command\n");
			  else
			  {
			    j = 0;
			    k = 1;
		        while ((j <= t) && (k != atoi(line)))
			    {
				  if (state[j] != 2)
				    k++;
				  j++;
			    }
		        if (j > t)
			      printf("No such command in background mode\n");
		        else
		    	{
				  if (state[j] == -1)
				  {
					kill(mas_pid[j], SIGSTOP);
					waitpid(mas_pid[j], NULL, 0);
					printf("[%4d  ] Stopped     ",  k);
	                l = 0;
                    while (mas_jobs[j][l] != '\n')
                    {
                      printf("%c",mas_jobs[j][l]);
                      l++;
                    }
		            printf("\n");  
				  }
				  else
				  {
			      pid = mas_pid[j];
                  waitpid(pid, NULL, WUNTRACED);
				  if (exitstate == 1) /*ctrl+z*/
		          {
			        printf("[%4d  ] Stopped     %s",  k, mas_jobs[j]);
	                l = 0;
                    while (mas_jobs[j][l] != '\n')
                    {
                      printf("%c",mas_jobs[j][l]);
                      l++;
                    }
		            printf("\n");	
			        if (t != (JOB_SIZE - 1)) /*If there are rooms in the history*/
                    {
	                  t++;
                      mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char))); /*Memory for the new command*/
	                  if (mas_jobs[t] == NULL) /*Error of memory for new command*/
                      {
                        perror("Error of memory for new command\n");
						for (j = 0; j <= t; j++)
		                  if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		                kill(mas_pid[j], SIGSTOP);
	  	                del_history(history);
			            del_history(mas_jobs);
       	        	    free(mas_pid);
			            free(state);
				        if (conv_next)
					      free(line);
		                close(inputfile);
                        close(outputfile);
                        if (conv_next == 1)
	                      close(channel[0]);
	                    if (conv_next == 2)
	                      close(channel2[0]);
		                dup2(input, 0);
                        dup2(output, 1);
                        close(input);
                        close(output);
	                    return 1;
                      }
                      mas_jobs[t + 1] = NULL; /*Next elements of history is empty*/
                    }
                    else /*If there are not rooms in the history*/
                    {
			          l = 0;
                      free(mas_jobs[j]); /*Delete the first command*/
                      while (l != (JOB_SIZE - 1)) /*Move all commands*/
                      {
                        mas_jobs[l] = mas_jobs[l + 1];
                        l++;
                      }
                      mas_jobs[l] = NULL; /*Next elements of history is empty*/
                      mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char)));
	                  if (mas_jobs[t] == NULL) /*Error of memory for new command*/
                      {
                        perror("Error of memory for new command\n");
						for (j = 0; j <= t; j++)
		                  if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		                kill(mas_pid[j], SIGSTOP);
	                    del_history(history);
			            del_history(mas_jobs);
       	        	    free(mas_pid);
			            free(state);
				        if (conv_next)
					      free(line);
		                close(inputfile);
                        close(outputfile);
                        if (conv_next == 1)
	                      close(channel[0]);
	                    if (conv_next == 2)
	                      close(channel2[0]);
		                dup2(input, 0);
                        dup2(output, 1);
                        close(input);
                        close(output);
	                    return 1;
                      }
	                }
                    l = 0;
                    while (((((job->next)->command)->arguments)[0][l] != '\n') && ((((job->next)->command)->arguments)[0][l] != ';')  && ((((job->next)->command)->arguments)[0][l] != '\0')) /*Input the command in the element of history*/
                    {
                      mas_jobs[t][l] = mas_jobs[j][l];
                      l++;
                      if (l >= buffer_size_job) /*Increase size of one string in the history*/
                      {
                        buffer_size_job = buffer_size_job + BUFFER_SIZE;
                        mas_jobs[t] = (char *) realloc (mas_jobs[t], buffer_size_job * sizeof(char));
                      }
		            }
                    mas_jobs[t][l] = '\n'; /*Last symbol every command*/
		            mas_pid = (pid_t *) realloc (mas_pid, (t + 1) * sizeof(pid_t));
		            mas_pid[t] = mas_pid[j];
			        state = (pid_t *) realloc (state, (t + 1) * sizeof(pid_t));
		            state[t] = -1;
		          }
		          }
				  pid = 1;
			      free(mas_jobs[j]);
		          l = j;
                  while (l != t)
                  {
                    mas_jobs[l] = mas_jobs[l + 1];
			        mas_pid[l] = mas_pid[l + 1];
			        state[l] = state[l + 1];
                    l++;
                  }
                  mas_jobs[t] = NULL;
	              t--;
			    }
		      }
			}
			else
			  printf("Error parametrs\n");
	      }
	      else /*So much arguments of fg*/
		    printf("So much arguments of fg");
	    }
	    }
	    else
	      printf("No such command in background mode\n");
	    if (conv_next)
		  free(line);
	  }
	  else
	  {
	  if (!strcmp((((job->next)->command)->arguments)[0], "bg")) /*bg*/
      {
		exitstate = 0;
		if (t != -1)
		{
		j = t;
		while ((j) && (state[j] != -1))
	      j--;
		if (state[j] != -1) /*No restoped command*/
		  printf("No command in waiting mode\n");
	    else
	    {
		  pid = mas_pid[j];
		  kill(pid, SIGCONT);
		  if (t != (JOB_SIZE - 1)) /*If there are rooms in the history*/
          {
	        t++;
            mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char))); /*Memory for the new command*/
	        if (mas_jobs[t] == NULL) /*Error of memory for new command*/
            {
              perror("Error of memory for new command\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
            }
            mas_jobs[t + 1] = NULL; /*Next elements of history is empty*/
          }
          else /*If there are not rooms in the history*/
          {
			k = 0;
            free(mas_jobs[k]); /*Delete the first command*/
            while (k != (JOB_SIZE - 1)) /*Move all commands*/
            {
              mas_jobs[k] = mas_jobs[k + 1];
              k++;
            }
            mas_jobs[k] = NULL; /*Next elements of history is empty*/
            mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char)));
	        if (mas_jobs[t] == NULL) /*Error of memory for new command*/
            {
               perror("Error of memory for new command\n");
			   for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
             }
	      }
          k = 0;
          while ((mas_jobs[j][k] != '\n') && (mas_jobs[j][k] != ';')  && (mas_jobs[j][k] != '\0')) /*Input the command in the element of history*/
          {
            mas_jobs[t][k] = mas_jobs[j][k];
            k++;
            if (k >= buffer_size_job) /*Increase size of one string in the history*/
            {
              buffer_size_job = buffer_size_job + BUFFER_SIZE;
              mas_jobs[t] = (char *) realloc (mas_jobs[t], buffer_size_job * sizeof(char));
            }
		  }
          mas_jobs[t][k] = '\n'; /*Last symbol every command*/
		  mas_pid = (pid_t *) realloc (mas_pid, (t + 1) * sizeof(pid_t));
		  mas_pid[t] = pid;
		  state = (pid_t *) realloc (state, (t + 1) * sizeof(pid_t));
		  state[t] = 1;
		  pid = 1;
		  free(mas_jobs[j]);
		  l = j;
          while (l != t)
          {
            mas_jobs[l] = mas_jobs[l + 1];
			mas_pid[l] = mas_pid[l + 1];
			state[l] = state[l + 1];
            l++;
          }
          mas_jobs[t] = NULL;
	      t--;
		}
	    }
		else
		  perror("No restoped proces\n");
	  }
	  else /*Extern command*/
	  {
		exitstate = 0;
		if (((job->next)->command)->background_flag) /*Background mode*/
		{
		if ((pid = fork()) == -1)
        {
              perror("Error with create new process\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
        }
        if (!pid) /*New process in background mode*/
		{
		  signal(SIGINT, SIG_IGN);
		  execvp((((job->next)->command)->arguments)[0], ((job->next)->command)->arguments);
          perror("Unknown command\n");
		  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
		}
        else /*Continue no waiting end of process*/
		{
		  if (t != (JOB_SIZE - 1)) /*If there are rooms in the history*/
          {
	        t++;
            mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char))); /*Memory for the new command*/
	        if (mas_jobs[t] == NULL) /*Error of memory for new command*/
            {
              perror("Error of memory for new command\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	          del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
            }
            mas_jobs[t + 1] = NULL; /*Next elements of history is empty*/
          }
          else /*If there are not rooms in the history*/
          {
			j = 0;
            free(mas_jobs[j]); /*Delete the first command*/
            while (j != (JOB_SIZE - 1)) /*Move all commands*/
            {
              mas_jobs[j] = mas_jobs[j + 1];
              j++;
            }
            mas_jobs[j] = NULL; /*Next elements of history is empty*/
            mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char)));
	        if (mas_jobs[t] == NULL) /*Error of memory for new command*/
            {
               perror("Error of memory for new command\n");
			   for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	           del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
             }
	      }
          j = 0;
          while (((((job->next)->command)->arguments)[0][j] != '\n') && ((((job->next)->command)->arguments)[0][j] != ';')  && ((((job->next)->command)->arguments)[0][j] != '\0')) /*Input the command in the element of history*/
          {
            mas_jobs[t][j] = (((job->next)->command)->arguments)[0][j];
            j++;
            if (j >= buffer_size_job) /*Increase size of one string in the history*/
            {
              buffer_size_job = buffer_size_job + BUFFER_SIZE;
              mas_jobs[t] = (char *) realloc (mas_jobs[t], buffer_size_job * sizeof(char));
            }
		  }
          mas_jobs[t][j] = '\n'; /*Last symbol every command*/
		  mas_pid = (pid_t *) realloc (mas_pid, (t + 1) * sizeof(pid_t));
		  mas_pid[t] = pid;
		  state = (pid_t *) realloc (state, (t + 1) * sizeof(pid_t));
		  state[t] = 1;
		  pid = 1;
		}
		}
		else /*Frontground mode*/
		{
	    if ((pid = fork()) == -1)
        {
              perror("Error with create new process\n");
			  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
        }
		if (!pid) /*New process in front mode*/
		{
		  execvp((((job->next)->command)->arguments)[0], ((job->next)->command)->arguments);
          perror("Unknown commandq\n");
		  for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		     close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
		}
	    else /*ctrl+z; ctrl+c; return 0*/
		{
		  waitpid(pid, NULL, WUNTRACED);
		  if (exitstate == 1) /*ctrl+z*/
		  {
			printf("[%4d  ] Stopped    %s\n",  t + 2, (((job->next)->command)->arguments)[0]);
			if (t != (JOB_SIZE - 1)) /*If there are rooms in the history*/
            {
	          t++;
              mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char))); /*Memory for the new command*/
	          if (mas_jobs[t] == NULL) /*Error of memory for new command*/
              {
                perror("Error of memory for new command\n");
				for (j = 0; j <= t; j++)
		        if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		      kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
              }
              mas_jobs[t + 1] = NULL; /*Next elements of history is empty*/
            }
            else /*If there are not rooms in the history*/
            {
			  j = 0;
              free(mas_jobs[j]); /*Delete the first command*/
              while (j != (JOB_SIZE - 1)) /*Move all commands*/
              {
                mas_jobs[j] = mas_jobs[j + 1];
                j++;
               }
               mas_jobs[j] = NULL; /*Next elements of history is empty*/
               mas_jobs[t] = (char *) malloc (buffer_size_job * (sizeof(char)));
	           if (mas_jobs[t] == NULL) /*Error of memory for new command*/
               {
                 perror("Error of memory for new command\n");
				 for (j = 0; j <= t; j++)
		           if (!waitpid(mas_pid[j], NULL, WNOHANG))
	 		         kill(mas_pid[j], SIGSTOP);
	  	      del_history(history);
			  del_history(mas_jobs);
       	      free(mas_pid);
			  free(state);
		      close(inputfile);
              close(outputfile);
              if (conv_next == 1)
	            close(channel[0]);
	          if (conv_next == 2)
	            close(channel2[0]);
		      dup2(input, 0);
              dup2(output, 1);
              close(input);
              close(output);
	          return 1;
               }
	        }
            j = 0;
            while (((((job->next)->command)->arguments)[0][j] != '\n') && ((((job->next)->command)->arguments)[0][j] != ';')  && ((((job->next)->command)->arguments)[0][j] != '\0')) /*Input the command in the element of history*/
            {
              mas_jobs[t][j] = (((job->next)->command)->arguments)[0][j];
              j++;
              if (j >= buffer_size_job) /*Increase size of one string in the history*/
              {
                buffer_size_job = buffer_size_job + BUFFER_SIZE;
                mas_jobs[t] = (char *) realloc (mas_jobs[t], buffer_size_job * sizeof(char));
              }
		    }
            mas_jobs[t][j] = '\n'; /*Last symbol every command*/
		    mas_pid = (pid_t *) realloc (mas_pid, (t + 1) * sizeof(pid_t));
		    mas_pid[t] = pid;
			state = (pid_t *) realloc (state, (t + 1) * sizeof(pid_t));
		    state[t] = -1;
		  }
		  pid = 1;
		}
	    }
	  }
	  }
	  }
	  }
	  }
	  }
	  }
	  if (((job->next)->command)->input != NULL)
	    close(inputfile);
	  if (((job->next)->command)->output != NULL)
        close(outputfile);
      if (conv_next == 1)
	    close(channel[0]);
	  if (conv_next == 2)
	    close(channel2[0]);
	  dup2(input, 0);
      dup2(output, 1);
      close(input);
      close(output);
	  job = job->next;
    }
  }
  return 0;
}
