#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <wchar.h>
#include <locale.h>
#include <sys/stat.h>

struct position /*Position of cursor: Cursor*/
{
  int X;
  int Y;
  int prevX;
  int prevY;
} cursor;

struct list_node /*Node of the list*/
{
  wchar_t *str;
  struct list_node *prev;
  struct list_node *next;
};

typedef struct /*Biderict_list*/
{
  size_t num_elements;
  struct list_node *head;
  struct list_node *tail;
} Biderect_list;

Biderect_list *createBiderect_list() /*Create list*/
{
  Biderect_list *tmp = (Biderect_list*) malloc(sizeof(Biderect_list)); /*Memory on the new list*/

  tmp->num_elements = 0;
  tmp->head = tmp->tail = NULL;

  return tmp;
}

void deleteBiderect_list(Biderect_list **list) /*Delete list*/
{
  struct list_node *tmp = (*list)->head;
  struct list_node *next = NULL;
  
  while(tmp)
  {
    next = tmp->next;
	free(tmp);
	tmp = next;
  }
  free(*list);
  (*list) = NULL;
}

void pushBack(Biderect_list *list, wchar_t* s, int maxlen) /*Push node in the list back*/
{
  struct list_node *tmp = (struct list_node *) malloc (sizeof (struct list_node));
  int i; /*Counter in the string*/

  if (tmp == NULL) /*Error create list*/
    exit(1);

  tmp->str = (wchar_t *) malloc (0); /*Memory on the new element*/

  i = 0;
  while(s[i] != L'\0')
  {
	tmp->str = (wchar_t *) realloc (tmp->str, sizeof (wchar_t) * (i + 1));
    tmp->str[i] = s[i];
	i++;
  }
  tmp->str = (wchar_t *) realloc (tmp->str, sizeof (wchar_t) * (i + 1));
  tmp->str[i] = L'\0';
  tmp->next = NULL;
  tmp->prev = list->tail;
  if (list->tail)
    (list->tail)->next = tmp;
  list->tail = tmp;
  if (list->head == NULL)
    list->head = tmp;

  list->num_elements++;
}

void popBack(Biderect_list *list) /*Pop last node in the list*/
{
  struct list_node *next;
  wchar_t *tmp;

  if (list->tail == NULL)
    exit(4);

  next = list->tail;
  list->tail = (list->tail)->prev;
  if (list->tail)
    list->tail->next = NULL;
  if (next == list->head)
    list->head = NULL;

  tmp = next->str;
  free(next);

  list->num_elements--;
}

/*Print list since f_row, Y. Return new f_row*/
int printBiderect_list_since_number(Biderect_list *list, int maxlen, int Y, int with_number, int X, int col, int f_col, int row)
{
  struct list_node *tmp = list->head;
  int first_cursor = 0, f_c;/*Number symbols in the number string*/
  int j = 0; /*Number of string*/
  int numstr; /*Auxiliary number of string*/
  int lines; /*Number strings in the file*/
  int i;/*Number of string symbol*/
  int X1;

  wprintf(L"\n");

  lines = list->num_elements;
  while (lines)/*Number symbols in the number string*/
  {
    lines = lines / 10;
	first_cursor++;
  }

  if (with_number)
    col = col - first_cursor - 1;
  col--;

  if ((X + 1) < f_col)
    f_col--;
  if (!(f_col + col - X - 1))
    f_col++;

  while (tmp)
  {
	j++;
	if ((j >= Y) && (j <= (row + Y - 2))) /*Print strings to begin since string with cursor*/
    {
	  if (f_col == 1)
	    wprintf(L"|");
	  else
	    wprintf(L"<");
      if (with_number) /*Print the number of string*/
      {
		f_c = first_cursor;
		numstr = j;
		wprintf(L"%d", numstr);
		while (numstr)
        {
          numstr = numstr / 10;
	      f_c--;
        }
		while (f_c)
	    {
		  wprintf(L" ");
		  f_c--;
	    }
		wprintf(L":");
	  }
      if (j == Y) /*Print string with cursor*/
	  {
		col--;
		i = 0;
		X1 = X;
		while ((tmp->str[i] != L'\0') && ((int)tmp->str[i] != 13))
	    {
		  if ((int)tmp->str[i] != 65279)
		  {
		    if (X1 == i)
		      wprintf(L"\x1b[1;32m|\x1b[0m\x1b[0;36m\x1b[0m");
		    if (((i + 1) >= f_col) && ((i + 1) <= (f_col + col - 1)))
	          wprintf(L"%lc", tmp->str[i]);
		  }
		  else
		    X1++;
		  i++;
	    }
		if (X1 >= i)
		{
		  while (X1 != i)
		  {
			if (((i + 1) >= f_col) && ((i + 1) <= (f_col + col - 1)))
			  wprintf(L" ");
		    i++;
		  }
		  wprintf(L"\x1b[1;32m|\n\x1b[0m\x1b[0;36m\x1b[0m");
		}
		else
		  wprintf(L"\n");
		col++;
	  }
	  else /*Print string without cursor*/
	  {
		i = 0;
		while (tmp->str[i] != L'\0')
		{
		  if (((i + 1) >= f_col) && ((i + 1) <= (f_col + col - 1)))
	        wprintf(L"%lc", tmp->str[i]);
		  i++;
	    }
        wprintf(L"\n");
	  }
	}
    tmp = tmp->next;
  }

  if (row <= (list->num_elements - Y + 2))
  {
	wprintf(L"%d(%d):", Y, (int)list->num_elements);
    return f_col;
  }
  for (j = 0; j < (row - list->num_elements + Y - 2); j++)
    wprintf(L"\n");
  wprintf(L"%d(%d):", Y, (int)list->num_elements);

  return f_col;
}

void outf_printBiderect_list_since_number(Biderect_list *list, int maxlen, int Y, int with_number, int X, int col)
{
  struct list_node *tmp = list->head;
  int first_cursor = 0, f_c;/*Number symbols in the number string*/
  int j; /*Number of string*/
  int numstr; /*Auxiliary number of string*/
  int lines; /*Number strings in the file*/
  int i;/*Number of string symbol*/

  rewind(stdout);
  for (j = 0; j <= (list->num_elements + 1)*(maxlen + 10); j++)
    wprintf(L"\t");

  rewind(stdout);

  lines = list->num_elements;
  while (lines)/*Number symbols in the number string*/
  {
    lines = lines / 10;
	first_cursor++;
  }

  j = 0;
  while (tmp)
  {
	j++;
    if (with_number) /*Print the number of string*/
    {
	  f_c = first_cursor;
	  numstr = j;
	  wprintf(L"%d", numstr);
	  while (numstr)
      {
        numstr = numstr / 10;
	    f_c--;
      }
	  while (f_c)
	  {
	    wprintf(L" ");
		f_c--;
	  }
	  wprintf(L":");
	}
    if (j == Y) /*Print string with cursor*/
	{
	  for (i = 0; i < (maxlen - 1); i++)
	  {
		if (X == i)
		  wprintf(L"|");
		wprintf(L"%lc", tmp->str[i]);
	  }
	  wprintf(L"\n");
	}
	else /*Print string without cursor*/
	{
	  for (i = 0; i < (maxlen - 1); i++)
	    wprintf(L"%lc",tmp->str[i]);
	  wprintf(L"\n");
	}
    tmp = tmp->next;
  }
  wprintf(L"%d(%d):", Y, (int)list->num_elements);
}

int searchSubstring(Biderect_list *list, wchar_t *sub, int maxlen, int sublen, int with_number, int sub_numb) /*Return cursor.Y*/
{
  struct list_node *tmp = list->head;
  int number = 0, numbersub = 0; /*Number of string*/
  int i, j; /*Counters*/
  int X; /*Cursor*/
  int length = 0;
  
  int first_cursor = 0, f_c;/*Number symbols in the number string*/
  int numstr; /*Auxiliary number of string*/
  int lines; /*Number strings in the file*/

  while (tmp)
  {
	number++;
	j = 0;
	if (!numbersub)
	{
	  i = 0;
	  while (tmp->str[i] != L'\0')
	  {
        if (tmp->str[i] == sub[j])
	    {
		  j++;
		  if (j == sublen)
		  {
			sub_numb--;
			if (!sub_numb)
			{
		      numbersub = number;
			  X = i - sublen + 2;
			}
		  }
	    }
	    else
	      j = 0;
	    i++;
      }
	}
	else
	  break;
    tmp = tmp->next;
  }

  return numbersub;
}

/*Same funstion to return position X*/
int cursorSearch(Biderect_list *list, wchar_t *sub, int maxlen, int sublen, int sub_numb) /*Return cursor.X*/
{
  struct list_node *tmp = list->head;
  int number = 0, numbersub = 0; /*Number of string*/
  int i, j; /*Counters*/
  int X, X1; /*Cursor*/
  int length = 0;
  
  int first_cursor = 0, f_c;/*Number symbols in the number string*/
  int numstr; /*Auxiliary number of string*/
  int lines; /*Number strings in the file*/

  while (tmp)
  {
	X1 = 0;
	number++;
	j = 0;
	if (!numbersub)
	{
	  i = 0;
	  while (tmp->str[i] != L'\0')
	  {
		if ((int)tmp->str[i] == 65279)
		  X1++;
        if (tmp->str[i] == sub[j])
	    {
		  j++;
		  if (j == sublen)
		  {
			sub_numb--;
			if (!sub_numb)
			{
		      numbersub = number;
			  X = i - sublen + 2;
			  if (X1)
			    X--;
			  }
		  }
	    }
	    else
	      j = 0;
	    i++;
      }
	}
	else
	  break;
    tmp = tmp->next;
  }
  X--;

  return X;  
}

void changeStrings(wchar_t *file_string, wchar_t *old_str, int old, wchar_t *new_str, int new, int empty, int txt) /*Return new length of strings*/
{
  int i = 0, j, k = 0; /*Counters*/
  int length_file = 0;

  while (file_string[length_file] != L'\0')
    length_file++;

  file_string[length_file] = L'\0';
  if (empty)
  {
	while (file_string[k] != L'\0')
	{
	  if (file_string[k] == L'\n')
	  {
		j = 0;
		i = k;
		i++;
		while (file_string[i] == L'\n')
		{
		  i++;
		  j++;
		}
		if (j)
		{
		  k++;
		  i = k;
		  if (new)
		  {
	        while ((i + j) < wcslen(file_string)) /*Delete empty strings*/
		    {
		      file_string[i] = file_string[i + j];
			  i++;
		    }
		    i = wcslen(file_string) - 1;
			while (i > (k + new * j)) /*Place for new strings and '\n'*/
			{
			  file_string[i] = file_string[i - new * j - 1];
			  i--;
			}
			while (j)
		    {
			  for (i = k; i < (k + new); i++)
			    file_string[i] = new_str[i - k];
			  j--;
			  k = k + new;
		    }
			file_string[k] = L'\n';
		  }
		  else
		  {
			while ((i + j) < wcslen(file_string))
			{
			  file_string[i] = file_string[i + j];
			  i++;
			}
		  }
		}
	  }
	  k++;
	}
  }
  else
  {
    i = 0;
    while (k < wcslen(file_string))
    {
	  j = 0;
	  i = k;
	  while (file_string[i] == old_str[j])
	  { 
	    i++;
	    j++;
	  }
      if (j == old) /*Rechange substrings*/
	  {
	    j = k;
	    while ((j + old) < wcslen(file_string)) /*Delete old substrings*/
	    {
          file_string[j] = file_string[j + old];
		  j++;
	    }
	    if (new) /*Add new substrings*/
	    {
	      j = (wcslen(file_string) - 1);
	      while (j >= k + new)
	      {
	        file_string[j] = file_string[j - new];
		    j--;
	      }
	      for (j = k; j < k + new; j++)
	        file_string[j] = new_str[j - k];
	      k = k + new - 1;
	    }
	  }
	  k++;
    }
  }
}

int countSubstr (wchar_t *str, wchar_t *substr, int empty) /*Number of substring in the string*/
{
  int i, j, t = 0, k = 0;

  if (empty)
  {
	while (str[k] != L'\0')
	{
	  if (str[k] == L'\n')
	  {
		k++;
		while (str[k] == L'\n')
		{
		  t++;
		  k++;
		}
	  }
	  k++;
	}
	return t;
  }
  while (str[k] != L'\0')
  {
	j = 0;
	i = k;
	while (str[i] == substr[j])
	{
	  i++;
	  j++;
	}
    if (j == wcslen(substr))
      t++;
	k++;
  }  

  return t;
}

int countEmpty (wchar_t *str) /*Number of empty string, that should be deleted*/
{
  int i = 0, empty = 0;

  while (str[i] != L'\0')
  {
	if (str[i] == L'\n')
	{
	  i++;
	  while (str[i] == L'\n')
	  {
	    i++;
	    empty++;
 	  }
	}
	i++;
  }  

  return empty;
}

int EmptySubstr (wchar_t *str) /*Number of group of empty strings*/
{
  int i = 0, empty = 0, e;

  while (str[i] != L'\0')
  {
	if (str[i] == L'\n')
	{
	  i++;
	  e = 0;
	  while (str[i] == L'\n')
	  {
	    i++;
	    e++;
 	  }
	  if (e)
	    empty++;
	}
	i++;
  }  

  return empty;
}

void clear(int output, int row, int num, int maxlen) /*Clear screen*/
{
  int j;
  char symb = '\t';

  if (!output)
  {
    rewind(stdout);
	for (j = 0; j <= (num + 1)*(maxlen + 10); j++)
      wprintf(L"%d", (int)symb);
  }
  else
    for (j = 0; j <= row; j++)
	  wprintf(L"\t\n");
}

int main (int argc, char** argv)
{
  struct termios old_attributes, new_attributes; /*Input from the terminal*/

  int opt = 0; /*Next option*/
  extern int optind; /* Index next symbols that is not options*/
  extern char *optarg; /*Options*/
  int par[3]; /* The parametrs of reading file (n, h, v)*/
  int i = 1; /*Number option is responsible of tha file name*/

  int j; /*Counters*/
  wchar_t symb; /*Auxiliary symbol*/

  FILE *f; /*Auxiliary files*/
  int len_file = 0; /*Length of the file*/
  wchar_t *file_string; /*File is written in one string*/
  int len = 0; /*Length string*/
  int maxlen = 0; /*Maximum number of symbols in the longest string in the file*/
  int lines = 0; /*Number of strings*/

  struct winsize wins; /*Size of the terminal*/

  wchar_t write[7];
  char *file_name; /*Name of the new file*/

  wchar_t *old_str;  /*For change strings*/
  wchar_t *new_str;  /*For change strings*/
  wchar_t *sb;
  int Y; /*New position of cursor*/
  int lines_count;
  int t;

  wchar_t* sub = (wchar_t *) malloc (sizeof (wchar_t));; /*Substring*/
  int row; /*Number of row in the terminal*/

  int r, nosub;
  int k; /*Counter in the string*/
  int number;
  int col; /*Number of row in the terminal*/
  int f_col = 1; /*Number of the first symbol that displayed in the terminal*/
	
  wchar_t symb2, n = '\n'; /*Auxiliary symbol*/

  int l; /*Counter for clean screen*/
  int symbol;

  int count = 0;
  int sub_numb = 1; /*Number of substraction*/
  int fl_n, empty;/*Number of '\n'*/
  int txt = 0;
  int ES; /*Empty substring in groups*/

  if(isatty(0))/*Input from the terminal*/
  {
    tcgetattr(0,&old_attributes);
    memcpy(&new_attributes,&old_attributes,sizeof(struct termios));
    new_attributes.c_lflag &= ~ECHO;
    new_attributes.c_lflag &= ~ICANON;
    new_attributes.c_cc[VMIN] = 1;
    tcsetattr(0,TCSANOW,&new_attributes);
  }

  if ((argc < 2) && (isatty(0))) /*Check to enter data*/
  {
    perror("No any data");
    if(isatty(0))
      tcsetattr(0,TCSANOW,&old_attributes);
    return 1;
  }

  setlocale(LC_ALL, "ru_RU.UTF-8"); /*Russion symbols*/

  for (j = 1; j <= 3; j++) /*Empty options*/
    par[j] = 0;

  opterr = 0; /*Not display error of options*/

  while ((opt = getopt(argc, argv, "n::h::v")) != -1) /*Read options*/
  {
    i = optind;
    switch (opt)
    {
      case 'n':
        par [1] = 1; /*Print file*/
        break;
      case 'h':
        par [2] = 1; /*Print less command*/
        break;
      case 'v':
        par [3] = 1; /*Print program*/
        break;
      case '?': /*Not options*/
        i--;
      break;
    }
  }

  if (par[3]) /*Versia of less*/
  {
    wprintf(L"It is Version 1.0 of programm");
	if (i < argc)
	{
	  if (par[1])
	    wprintf(L" with numbers\n");
      else
	    wprintf(L" without numbers\n");
    }
	if (par[2])
      wprintf(L" that print less command");	
    wprintf(L"\nInput ENTER to move to the next option:");
	while ((symb = getwchar()) != '\n');
	wprintf(L"\n");
	if (!isatty(1)) /*Clear screen*/
	{
	  rewind(stdout);
	  for (j = 0; j <= 81; j++)
		  wprintf(L"\t");
	}
   }

  if (par[2]) /*Print less command*/
  {
    wprintf(L"\nSUMMARY OF LESS COMMANDS\n\n");
	wprintf(L"OPTIONS\n\n");
    wprintf(L"Most options may be changed either on the command line:\n\n");
    wprintf(L"-n  - To number subsequent strings\n");
    wprintf(L"-v  - To print code of this program\n");
    wprintf(L"-h  - Display this help\n\n");
	wprintf(L"MOVING\n\n");
	wprintf(L"RightArrow - Move cursor right one character\n");
	wprintf(L"LeftArrow - Move cursor left one character\n");
	wprintf(L"UpArrow - Move cursor up one character\n");
	wprintf(L"DownArrow - Move cursor down one character\n\n");
	wprintf(L"ACTIONS\n\n");
	wprintf(L"/substring_fo_the_search – searches the text down to the closest finding. If the text is found, the \"cursor\"");
    wprintf(L"is translated in this place and position since which found find.");
    wprintf(L"If the substring to search for is empty, then the search is performed with the latest");
	wprintf(L"introduction of the substring,until the next occurrence.\n");
	wprintf(L"subst /string_example/on_this_replace/ – searches for with the replacement text.");
	wprintf(L"It is also necessary to provide shielding / character and other important characters, such as \"\\\" and \"/\" row screens /.");
	wprintf(L"number - means to move the cursor to the position specified by number. Line are numbered starting from one.\n");
    wprintf(L"write \"filename\" – saves the possible modified text command subst to a file with the specified name.\n");
    wprintf(L"\nYou should write the name of file that should be processed and options through the space\n\n");
    wprintf(L"\nInput ENTER to move to the next option:");
    while ((symb = getwchar()) != '\n');
	wprintf(L"\n");
    if (!isatty(1)) /*Clear screen*/
	{
	  rewind(stdout);
	  for (j = 0; j <= 1236; j++)
		  wprintf(L"\t");
	}
  }

  if (i >= argc) /*Not name of file*/
  {
    if(isatty(0))
      tcsetattr(0,TCSANOW,&old_attributes);
    wprintf(L"\n");
	return 0;
  }

/*Count length of the longest string and number of strings*/
  file_string = (wchar_t *) malloc (0);
  if (isatty(0)) /*Input from the terminal*/
  {
    f = fopen (argv[i], "rt");
	file_string = (wchar_t *) malloc (0);
    if (f == NULL) /*Error of openning file*/
    {
      perror("Error openning file");
      if(isatty(0))
        tcsetattr(0,TCSANOW,&old_attributes);
      return 1;
    }
	while ((symb = getwc(f)) != EOF) /*Count max length of longest string, lines*/
    {
	  if ((int)symb == 13)
	    continue;
      len_file++;
      file_string = (wchar_t *) realloc (file_string, len_file * sizeof (wchar_t));
	  file_string[len_file - 1] = symb;
	  len++;
      if (symb == L'\n')
      {
        if (len > maxlen)
          maxlen = len;  /*Max symbols in the longest string*/
        len = 0;
        lines++; /*Number of strings*/
      }
    }
  }
  else
  {
	while ((symb = getwchar()) != EOF) /*Count max length of longest string, lines*/
    {
	  if ((int)symb == 13)
	    continue;
      len_file++;	  
      file_string = (wchar_t *) realloc (file_string, len_file * sizeof (wchar_t));
	  file_string[len_file - 1] = symb;
	  len++;
      if ((int)symb == 10)
      {
        if (len > maxlen)
          maxlen = (len + 1);  /*Max symbols in the longest string*/
        len = 0;
        lines++; /*Number of strings*/
      }
    }

  }
  if (!len_file) /*Empty file*/
  {
	file_string = (wchar_t *) realloc (file_string, sizeof (wchar_t));
	file_string[0] = '\0' ;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &wins);/*Determine sizes of the terminal*/
	row = wins.ws_row;
	if (isatty(0))
	  fclose(f);

	while(1)
	{
	  clear(isatty(1), row, 2, 0);

	  wprintf(L"0(0)\n");
      wprintf(L"It is empty file, so there are only commands to exit and to save\n");
	  wprintf(L":");

	  while ((int)(symb = getwchar()) == -1);

	  if (symb == L'\004')
	    break;
	  if (symb == L'\n')
		continue;

      if (symb == L'w') /*Save new file*/
	  {
		wprintf(L"w");
		write[0] = symb;
		j = 0;
	    while (((symb = getwchar()) != L'\n') && (j < 5) && (symb != L'\004'))
		{
		  wprintf(L"%lc", symb);
		  j++;
		  write[j] = symb;
		}
		if (symb == L'\004')
	      break;
		if (symb == L'\n')
		{
		  clear(isatty(1), row, 2, 0);
	      wprintf(L"\nNot such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
	    }
		j++;
		write[j] = L'\0';
		if (wcscmp(write, L"write ")) /*Not "write "*/
		{
	      clear(isatty(1), row, 2, 0);
	      wprintf(L"\nNo such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
		if (symb != L'"')/*Not "write ""*/
		{ 
		  clear(isatty(1), row, 2, 0);
		  wprintf(L"\nNo such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
	    else /*Read the file name*/
		{
		  wprintf(L"\"");
		  symb = getwchar();
		  j = 0;
		  file_name = (char *) malloc (0);
		  while ((symb != L'"') && (symb != L'\n') && (symb != L'\004'))
		  {
			wprintf(L"%lc", symb);
			file_name = (char *) realloc (file_name, (j + 1) * sizeof (char));
			file_name[j] = (char) symb;
			j++;
		    symb = getwchar();
		  }
		  if (symb == L'\004')
		  {
			free(file_name);
	        break;
		  }
  		  if (symb == L'\n') /*write "name*/
		  {
			free (file_name);
			clear(isatty(1), row, 2, 0);
			wprintf(L"\nNo such command\n");
		    wprintf(L"Input ENTER:");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
		    continue;
		  }
          file_name = (char *) realloc (file_name, (j + 1) * sizeof (char));
          file_name[j] = '\0';
		  wprintf(L"\"");
		  symb = getwchar();
		  if (symb != L'\n')
		  {
			free(file_name);
		  	if (symb == L'\004')
	          break;
			clear(isatty(1), row, 2, 0);
			wprintf(L"\nUnknown command\nInput ENTER:");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
			continue;
		  }
		  f = fopen (file_name, "wt");
		  for (j = 0; j < 1; j++)
		    putwc(file_string[j], f);
		  fclose(f);
		  free (file_name);
		  continue;
		}
	  }

	  if (symb == L'q') /*Exit*/
	  {
		wprintf(L"q");
	    symb = getwchar();
		if (symb == L'\004')
	      break;
		if (symb == L'\n')
	      break;
	    else
		{
		  clear(isatty(1), row, 2, 0);
		  wprintf(L"\nUnknown command.\nInput ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
	  }

      wprintf(L"\nUnknown command\n");
      wprintf(L"Input ENTER:");
	  while (((int)(symb) != 10) && (symb != L'\004'))
		symb = getwchar();
	  wprintf(L"\n");
	}
	if(isatty(0))
      tcsetattr(0,TCSANOW,&old_attributes);
	free(file_string);
	wprintf(L"\n");
    return 0;
  }

  if (((int)file_string[len_file - 2] != 10) && ((int)file_string[len_file - 2] != 13))/*Last symbol is not '\n'*/
  {
	len++;
    if (len > maxlen)
      maxlen = len;  /*Max symbols in the longest string*/
	lines++;
  }
  len_file++;
  file_string = (wchar_t *) realloc (file_string, (len_file + 1) * sizeof (wchar_t));
  file_string[len_file - 1] = L'\0';

/*Work with file*/
  {
	wchar_t s[maxlen]; /*String from the file*/
    Biderect_list *list = createBiderect_list();

	cursor.X = 0;
	cursor.Y = 1;
	/*Write file in the list*/
	i = 0;
	k = 0;
	while (file_string[i] != L'\0')
	{
	  if (file_string[i] == L'\n')
	  {
		if (i)
		  if ((int)file_string[i -1] == 13)
		    txt = 1;
		s[k] = L'\0';
		pushBack(list, s, maxlen);
	    k = 0;
	  }
	  else
	  {
	    s[k] = file_string[i];
	    k++;
	  }
	  i++;
	}
	if (file_string[i - 1] != '\n')
	{
	  s[k] = L'\0';
	  pushBack(list, s, maxlen);
	}
    if (isatty(0))
      fclose(f);

	while(1)
	{
	  /*PRINT*/
	  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wins);/*Determine sizes of the terminal*/
      col = wins.ws_col;
	  row = wins.ws_row;
	  if (isatty(1))
	    f_col = printBiderect_list_since_number(list, maxlen, cursor.Y, par[1], cursor.X, col, f_col, row);
      else
	    outf_printBiderect_list_since_number(list, maxlen, cursor.Y, par[1], cursor.X, col); 
      /*Actions with file*/
	  cursor.prevX = cursor.X;
      cursor.prevY = cursor.Y;

	  while ((int)(symb = getwchar()) == -1);

	  if (symb == L'\004')
	    break;

	  if ((int)symb == 10)
	    continue;

	  if (symb == L'/') /*Search the /substring*/
	  {
		wprintf(L"/");
		if ((int)(symb = getwchar()) == 10) /*Empty string*/
		{
		  if ((sub_numb - 1)) /*Not any substring*/
		  {
		    if (searchSubstring(list, sub, maxlen, count, par[1], sub_numb))
		    {
		      cursor.prevX = cursor.X;
		      cursor.Y = searchSubstring(list, sub, maxlen, count, par[1], sub_numb);
		      cursor.X = cursorSearch(list, sub, maxlen, count, sub_numb);
		      sub_numb++;
			  f_col = cursor.X + 2;
		      if (((int)(list->head)->str[0] == 65279) && (cursor.Y == 1) && (cursor.X))
		        f_col++;
		      continue;
			}
		  }
		  count = 0;
		  sub_numb = 1;
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nNot found\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  cursor.X = 0;
		  cursor.Y = 1;
		  f_col = 1;
		  continue;
		}
        free(sub);
        sub = (wchar_t *) malloc (sizeof (wchar_t));
		sub_numb = 1;
		count = 0;
		nosub = 0;
		while (((int)symb != 10) && (symb != L'\004'))
		{
		  wprintf(L"%lc", symb);
		  if (count >= (maxlen - 1))
		  {
			nosub = 1;
			break;
		  }
          sub = (wchar_t *) realloc (sub, (count + 1) * sizeof (wchar_t));
		  sub[count] = symb;
		  count++;
		  symb = getwchar();
		}
		if (nosub)
		{
		  count = 0;
		  sub_numb = 1;
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nNot found\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  cursor.X = 0;
		  cursor.Y = 1;
		  f_col = 1;
		  continue;
		}
		sub = (wchar_t *) realloc (sub, (count + 1) * sizeof (wchar_t));
		sub[count] = L'\0';
		if (symb == L'\004')
		  break;
		if (searchSubstring(list, sub, maxlen, count, par[1], sub_numb))
		{
	      cursor.prevX = cursor.X;
		  cursor.Y = searchSubstring(list, sub, maxlen, count, par[1], sub_numb);
		  cursor.X = cursorSearch(list, sub, maxlen, count, sub_numb);
		  sub_numb++;
		  f_col = cursor.X + 2;
		  if (((int)(list->head)->str[0] == 65279) && (cursor.Y == 1) && (cursor.X))
		    f_col++;
		  continue;
		}
		else
		{
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nNot found\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  cursor.X = 0;
		  cursor.Y = 1;
		  f_col = 1;
		  continue;
		}
	  }

	  if (symb == L's') /*Change strings "subst"*/
	  {
		wprintf(L"s");
		symb = getwchar();
		j = 0;
		sb = (wchar_t *) malloc (sizeof (wchar_t));
	    while ((symb != L'\n') && (symb != L'/') && (j <= 6) && (symb != L'\004'))
	    {
		  wprintf(L"%lc", symb);
		  sb = (wchar_t *) realloc (sb, (j + 1) * sizeof (wchar_t));
		  sb[j] = symb;
		  symb = getwchar();
		  j++;
		}
		sb = (wchar_t *) realloc (sb, (j + 1) * sizeof (wchar_t));
		sb[j] = L'\0';
		if (symb == L'\n')
		{
		  free(sb);
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nNot such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
		if (symb == L'\004')
		{
		  free(sb);
		  break;
		}
		if ((!wcscmp(sb, L"ubst ")) && (symb == L'/')) /*Change strings*/
		{
		  free(sb);
		  old_str = (wchar_t *) malloc (sizeof (wchar_t));
		  wprintf(L"/");
		  j = 0;
		  fl_n = 0;
		  symb = getwchar();
		  while ((symb != L'\n') && ((int)symb != 47) && (symb != L'\004')) /*Old string*/
		  {
			wprintf(L"%lc", symb);
		    if ((int)symb == 92) /*'\n' or not*/
			{
		      symb2 = getwchar();
			  wprintf(L"%lc", symb2);
			  if (symb2 == L'n') /*Print '\n'*/
			  {
				fl_n++;
				old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
			    old_str[j] = (wchar_t)10;
			  }
			  else
			  {
				if (symb2 == 47) /*Print '/'*/
			    {
				  old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
				  old_str[j] = (wchar_t)47;
				}
				else
			    {
				  old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
				  old_str[j] = symb;
				  j++;
		          old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
				  old_str[j] = symb2;
				}
			  }
			}
			else /*Read symbol*/
			{
		      old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
			  old_str[j] = symb;
			}
			j++;
			symb = getwchar();
		  }
		  old_str = (wchar_t *) realloc (old_str, (j + 1) * sizeof (wchar_t));
		  old_str[j] = L'\0';
		  if (symb == L'\n')
		  {
			free(old_str);
			clear(isatty(1), row, list->num_elements, maxlen);
			wprintf(L"\nNo such command");
		    wprintf(L"\nInput ENTER:");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
		    continue;
		  }
		  if (symb == L'\004')
		  {
		    free(old_str);
			break;
		  }
		  wprintf(L"/");
		  /*New string*/
		  k = 0;
		  new_str = (wchar_t *) malloc (sizeof (wchar_t));
		  symb = getwchar();
		  while ((symb != L'\n') && ((int)symb != 47) && (symb != L'\004')) /*Old string*/
		  {
			wprintf(L"%lc", symb);
		    if ((int)symb == 92) /*'\n' or not*/
			{
		      symb2 = getwchar();
			  wprintf(L"%lc", symb2);
			  if (symb2 == L'n') /*Print '\n'*/
			  {
				new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
			    new_str[k] = (wchar_t)10;
			  }
			  else
			  {
				if (symb2 == 47) /*Print '/'*/
				{
				  new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
				  new_str[k] = (wchar_t)47;
				}
				else
			    {
				  new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
				  new_str[k] = symb;
				  k++;
		          new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
				  new_str[k] = symb2;
				}
			  }
			}
			else /*Read symbol*/
			{
		      new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
			  new_str[k] = symb;
			}
			k++;
			symb = getwchar();
		  }
		  new_str = (wchar_t *) realloc (new_str, (k + 1) * sizeof (wchar_t));
		  new_str[k] = L'\0';
		  if (symb == L'\n')
		  {
			free(new_str);
			free(old_str);
			clear(isatty(1), row, list->num_elements, maxlen);
			wprintf(L"\nNot such command\n");
		    wprintf(L"Input ENTER:");
			while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
		    continue;
		  }
		  if (symb == L'\004')
		  {
		    free(old_str);
			free(new_str);
			break;
		  }
		  wprintf(L"/");
		  if ((int)(symb = getwchar()) != 10)
		  {
			free(old_str);
			free(new_str);
			if (symb == L'\004')
	          break;
			clear(isatty(1), row, list->num_elements, maxlen);
			wprintf(L"/nInput ENTER");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
			continue;
		  }
		  empty = 0;
		  if (fl_n == wcslen(old_str))
		    empty = 1;
		  t = countSubstr(file_string, old_str, empty);
		  l = wcslen(file_string);
		  if (empty)
		  {
		    j = 1;
			t = countEmpty(file_string);
			ES = EmptySubstr(file_string);
			if (k)
		    {
			  file_string = (wchar_t *) realloc (file_string, (l + (k - 1) * t + ES) * sizeof (wchar_t));
			  for (i = l; i < (l + (k - 1) * t + ES); i++)
			    file_string[i] = L' ';
		      file_string[l + (k - 1) * t + ES] = L'\0';
			}
		  }
		  else
		  {
		    if (k > j) /*Increase size the file_string*/
		    {
              file_string = (wchar_t *) realloc (file_string, (l + t * (k - j) + 1) * sizeof (wchar_t));
			  for (i = l; i < l + t * (k - j); i++)
			    file_string[i] = L' ';
		      file_string[l + t * (k - j)] = L'\0';
		    }
		  }
		  changeStrings(file_string, old_str, j, new_str, k, empty, txt); /*Return number deleted empty strings*/
		  if (!empty)
		  {
		    if (k < j)
		      file_string[l + t * (k - j)] = L'\0';
		  }
		  else
		  {
			if (!k)
			  file_string[l - t] = L'\0';
		  }
		  free(old_str);
		  free(new_str);
		  i = 0;
		  lines = 0;
		  len = 0;
		  maxlen = 0;
		  while (file_string[i] != L'\0')
		  {
			len++;
		    if (file_string[i] == L'\n')
			{
			  lines++;
			  if (len > maxlen)
			    maxlen = len;
			  len = 0;
			}
		    i++;
		  }
		  l = list->num_elements;
		  while (l)
		  {
			popBack(list);
		    l--;
		  }
		  cursor.X = 0;
		  cursor.Y = 1;
		  f_col = 1;
		  /*WRITE*/
		  i = 0;
	      k = 0;
	      while (file_string[i] != L'\0')
	      {
	        if (file_string[i] == L'\n')
	        {
		      s[k] = L'\0';
		      pushBack(list, s, maxlen);
	          k = 0;
	        }
	        else
	        {
	          s[k] = file_string[i];
	          k++;
	        }
	        i++;
	      }
	      if (file_string[i - 1] != '\n')
	      {
	        s[k] = L'\0';
	        pushBack(list, s, maxlen);
	      }
	      continue;
		}
		else /*Not change strings*/
	    {
		  free(sb);
		  clear(isatty(1), row, list->num_elements, maxlen);
	      wprintf(L"\nNo such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
	  }

      if (symb == L'w') /*Save new file*/
	  {
		wprintf(L"w");
		write[0] = symb;
		j = 0;
	    while (((symb = getwchar()) != L'\n') && (j < 5) && (symb != L'\004'))
		{
		  wprintf(L"%lc", symb);
		  j++;
		  write[j] = symb;
		}
		if (symb == L'\004')
	      break;
		if (symb == L'\n')
		{
		  clear(isatty(1), row, list->num_elements, maxlen);
	      wprintf(L"\nNot such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
	    }
		j++;
		write[j] = L'\0';
		if (wcscmp(write, L"write ")) /*Not "write "*/
		{
	      clear(isatty(1), row, list->num_elements, maxlen);
	      wprintf(L"\nNo such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
		if (symb != L'"')/*Not "write ""*/
		{ 
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nNo such command\n");
		  wprintf(L"Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
	    else /*Read the file name*/
		{
		  wprintf(L"\"");
		  symb = getwchar();
		  j = 0;
		  file_name = (char *) malloc (0);
		  while ((symb != L'"') && (symb != L'\n') && (symb != L'\004'))
		  {
			wprintf(L"%lc", symb);
			file_name = (char *) realloc (file_name, (j + 1) * sizeof (char));
			file_name[j] = (char) symb;
			j++;
		    symb = getwchar();
		  }
		  if (symb == L'\004')
		  {
			free(file_name);
	        break;
		  }
  		  if (symb == L'\n') /*write "name*/
		  {
			free (file_name);
			clear(isatty(1), row, list->num_elements, maxlen);
			wprintf(L"\nNo such command\n");
		    wprintf(L"Input ENTER:");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
		    continue;
		  }
          file_name = (char *) realloc (file_name, (j + 1) * sizeof (char));
          file_name[j] = '\0';
		  wprintf(L"\"");
		  symb = getwchar();
		  if (symb != L'\n')
		  {
			free(file_name);
		  	if (symb == L'\004')
	          break;
			clear(isatty(1), row, list->num_elements, maxlen);
			wprintf(L"\nUnknown command\nInput ENTER:");
		    while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		    if (symb == L'\004')
	          break;
			continue;
		  }
		  f = fopen (file_name, "wt");
		  for (j = 0; j < wcslen(file_string); j++)
		    putwc(file_string[j], f);
		  fclose(f);
		  free (file_name);
		  continue;
		}
	  }

	  if (symb == L'q') /*Exit*/
	  {
		wprintf(L"q");
	    symb = getwchar();
		if (symb == L'\004')
	      break;
		if (symb == L'\n')
	      break;
	    else
		{
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nUnknown command.\nInput ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
	  }

	  if ((symb >= L'0') && (symb <= L'9')) /*Change number of cursor string*/
	  {
		Y = 0;
		while ((symb != L'\n') && (symb >= L'0') && (symb <= L'9') && (symb != L'\004'))
		{ 
          wprintf(L"%c", symb);
		  Y = Y * 10 + ((int)symb) - ((int)'0');
		  symb = getwchar();
		}
        if (symb == L'\004')
	      break;
		if (symb != L'\n')
		{
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"\nUnknown symbol. Error. Input ENTER:");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		  continue;
		}
        if (Y > list->num_elements)
		{
		  clear(isatty(1), row, list->num_elements, maxlen);
		  wprintf(L"There was not such string.\nInput ENTER: ");
		  while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		  if (symb == L'\004')
	        break;
		}
		else
		  cursor.Y = Y;
  	    continue;
	  }

	  if ((int)symb != 27)
	  {
		clear(isatty(1), row, list->num_elements, maxlen);
		wprintf(L"\nUnknown command\n");
		wprintf(L"Input ENTER:");
		while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		if (symb == L'\004')
	      break;
	    continue;
	  }

	  symb = getwchar();
	  if (symb == L'\004')
	    break;
	  if ((int)symb != 91)
	  {
		clear(isatty(1), row, list->num_elements, maxlen);
		wprintf(L"\nUnknown command\n");
		wprintf(L"Input ENTER:");
		while (((int)(symb = getwchar()) != 10) && (symb != L'\004'));
		if (symb == L'\004')
	      break;
	    continue;
	  }

	  symb = getwchar();
	  if (symb == L'\004')
	    break;

   	  switch (symb) /*Arrows*/
      {
        case 65: /*65 - code up arrow*/
        cursor.Y--;
        if (cursor.Y < 1)
          cursor.Y = 1;
        break;
        case 68: /*68 - code left arrow*/
        cursor.X--;
        if (cursor.X <= 0)
        {
          cursor.X = 0;
		  f_col = 1;
        }
        break;
        case 67: /*67 - code right arrow*/
        cursor.X++;
        if (cursor.X > (maxlen - 1))
		  cursor.X = maxlen - 1;
        break;
        case 66: /*66 - code down arrow*/
        cursor.Y++;
		if (cursor.Y > list->num_elements)
	      cursor.Y = list->num_elements;
        break;
        default:
		clear(isatty(1), row, list->num_elements, maxlen);
		wprintf(L"\nUnknown command");
      }	  
	  wprintf(L"%d\n", cursor.X);
	}

	if(isatty(0))
      tcsetattr(0,TCSANOW,&old_attributes);
    free (file_string);
    if (isatty(1))	
      wprintf(L"\n");
    else
	  outf_printBiderect_list_since_number(list, maxlen, cursor.Y, par[1], cursor.X, col); 
    deleteBiderect_list(&list);
    free(sub);
    return 0;
  }
}
