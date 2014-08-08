/**
 * shell.c
 *
 * PURPOSE:		This is a simple shell which provides the option where 
 *				the up and down arrow keys may be used to scroll through
 * 				the command history.
 *
 * TO COMPILE:		gcc -Wall shell.c -o prog
 * TO RUN:		./prog
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/wait.h>

#define MAX_LENGTH 50
#define ARG_LENGTH 50
#define HISTORY_SIZE 100

enum BOOL{
	false = 0,
	true = 1
};
typedef enum BOOL boolean;

struct ARGUMENT{
	char *var_name;
	char *var_value;
};
typedef struct ARGUMENT Argument;


//use termios to manipulate terminal settings
static struct termios default_setting;

void default_terminal_setting(void)
{
    tcsetattr(0, TCSANOW, &default_setting);
}

void process_keypress(void)
{
    struct termios new_setting;

    tcgetattr(0, &default_setting);  
    new_setting = default_setting;  
    //set terminal to canonical mode and no echo since don't want to echo up and down keys
    //(this means user cannot modify input and echo must be done manually)
    new_setting.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(0, TCSANOW, &new_setting); 
    atexit(default_terminal_setting);
}

/**
 * PURPOSE: Search for the variable's actual value. Then replace the variable with
 *		  it's value in the args[]. This is the args[] that will be run in execvp().
 *		  The user has the option of combining multiple variables together: 
 *		  ie. In commandline: $variable1 $variable2 $variable3
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user.
 *		arg_count		The number of arguments.
 * 		ptr:			A pointer to the array of Arugment structs.
 *        s_index:		The size of the array of Argument structs.
 * 
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void process_variable_replacement(char *args[], int arg_count, Argument *ptr, int *s_index){


	boolean to_replace = false;
	
	char *variable;
	int arg_index;
	int row;
	int i;
	int index;
	
	if(args[0] != NULL){
		if(strcmp(args[0], "set") != 0){
	
			//find which argument the variable is
			for(row = 0; row < arg_count; row ++){
				if(args[row][0] == '$'){

					variable = strdup(args[row]);
					arg_index = row;
			
					//search the value that corresponds with our variable
					for(i = 0; i < (*s_index); i ++){
						if(strcmp(ptr[i].var_name, variable) == 0){
						index = i;
						to_replace = true;

						}

					}
					if(to_replace == true){
						//replace the variable with it's specified value as part of arguments
						args[arg_index] = strdup(ptr[index].var_value);
						to_replace = false;
					}
				}
			}
		
		}
	}

	

}

/**
 * PURPOSE: Processes "set" commands. If command is valid it gets stored in arguments
 *          for future reference.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user.
 *		arguments:	A pointer to the array of Arugment structs.
 *        s_index:		The size of the array of Argument structs.
 * 
 * OUTPUT PARAMETERS:
 *		s_index:		Pass back the size of array of Argument since it 
 *                       may have been updated.
 *
 */
void process_set_command(char *args[], Argument *arguments, int *s_index){

	char variable[MAX_LENGTH];
	char value[MAX_LENGTH];
	int variable_index = 0;
	int value_index = 0;
	int col = 0;

	boolean append_variable = true;
	boolean append_value = false;
	boolean invalid = false;
	
	//main processing
	if(args[0] != NULL && args[1] != NULL && args[2] == NULL){
		if(strcmp(args[0], "set") == 0 && args[1][0] == '$'){
	
			int length = strlen(args[1]);
			//if someone wrote "set $" is invalid
			if(length == 1){

				invalid = true;
			}
			col = 0;
			variable_index = 0;
			value_index = 0;
			
			//run through arg[1] which contains both variable and value assigned to it
			for(col = 0; col < length; col ++){
				
				if(append_variable == true){
					variable[variable_index] = args[1][col];
					variable_index ++;
				}
				if(append_value == true){

					value[value_index] = args[1][col];
					value_index ++;
				}
				
				if(args[1][col] == '='){
					variable[col] = '\0';
					append_variable = false;
					append_value = true;
				}
			

			}
			value[value_index] = '\0';
			
			if(append_value == false){
				
				invalid = true;
			}
			
			if(invalid == false){
				//put all values in a struct for reference later
				arguments[(*s_index)].var_name = strdup(variable);
				arguments[(*s_index)].var_value = strdup(value);
				(*s_index) = (*s_index) + 1;
			}
			else{
				printf("Invalid command\n");
			}
		}
		else{
			printf("Invalid command\n");
		}
	}
	else{
		printf("Invalid command\n");
	}

	
}

/**
 * PURPOSE: Checks if the command is a "set" command or a variable command.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user.
 *		arg_count		The number of arguments.
 *		arguments:	A pointer to the array of Arugment structs.
 *        s_index:		The size of the array of Argument structs.
 * 
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void check_if_set_command(char *args[], int arg_count, Argument *arguments, int *s_index){

	//show usage
	if(args[0] != NULL && args[1] == NULL){
		if(strcmp(args[0], "set") == 0){
			printf("Usage: set $variablename=value\n");	
		}
	
	}
	
	//process "set" commands
	if(args[0] != NULL && args[1] != NULL){
		if(strcmp(args[0], "set") == 0){
			process_set_command(args, arguments, s_index);	
		}
		
	}
	
	//process possible variables, not a set command
	if(args[0] != NULL){
		if(strcmp(args[0], "set") != 0){	
		
			process_variable_replacement(args, arg_count, arguments, s_index);
			
		}
	}

}


/**
 * PURPOSE: Used to shift the history when a user has used up all space
 *		  for the history. It removes the oldest entry in the history at 
 *          the start of the array, shifts all other entries and places the 
 *          newest entry at the end of the array.
 * 
 * INPUT PARAMETERS:
 *		history[]:	The history of commands entered.
 *		new_entry:	The new entry to put in history.
 * 
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void shift_history(char *history[], char *new_entry){
	
	int i;
	for(i = 0; i < HISTORY_SIZE-1; i ++){
		
		history[i] = history[i+1];
	}
	history[HISTORY_SIZE-1] = strdup(new_entry);

}

/**
 * PURPOSE: To iterate through the history.
 * 
 * INPUT PARAMETERS:
 *		ptr[]:		The history of commands entered.
 *		size:	     The size of the history.
 *        pos:		     The current position in the iteration.
 *        up:			If the arrow key is up or not.
 *        up_first:      If it is the first time arrow key up is pressed.
 * 
 * OUTPUT PARAMETERS:
 *		pos:		     The most updated position in the iteration.
 *
 */
int iterate_history(char *ptr[], int size, int pos, boolean up, boolean up_first){
	
	if(ptr != NULL){
		if(up == true){
	
			if(up_first == true && (size-1) >= 0){
				pos = size-1;
			}
			if(up_first == false){
				pos --;
				if(pos < 0){
					pos = 0;
				}
			}
			if(size > 0){
				if(pos >= 0 && pos < size){
					if(ptr[pos] != NULL){
                        //the \r and the extra spaces are used to clear the current line when 
                        //displaying a new command history
                        printf("\r                                                                 ");
						printf("\r%s", ptr[pos]);
					}
				}
			}
	
		}
		if(up == false){
			pos ++;
			if(pos >= size && (size-1) >= 0){
				pos = size-1;
			}
			if(size > 0){
				if(pos >= 0 && pos < size){
					if(ptr[pos] != NULL){
						printf("\r                                                                  ");
						printf("\r%s", ptr[pos]);
					}
				}
			}
		}
	}
	return pos;
}

/**
 * PURPOSE: Tokenize a line of text from a user.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user. Arguments 
 *					are to be stored here.
 *		input[]:		The line of text from a user.
 * 
 * OUTPUT PARAMETERS:
 *		arg_count:	The number of arguments.
 *
 */
int tokenize_input(char *args[], char input[]){

	int count = 0;
	int arg_count = 0;

	if(input != NULL){
		args[0] = strtok(input," ");

		while (args[count] != NULL) {
	
			count++;
			args[count] = strtok(NULL, " ");
			arg_count++;
		
		} 
	
		//did this to cut off any extra commands from a previous command
		//that was longer *this is very important
		count++;
		args[count] = '\0';
	}
	
	return arg_count;
	
}

/**
 * PURPOSE: Tokenize a line from the history.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user. Arguments 
 *					are to be stored here.
 *		history[]:	The history of commands entered.
 * 		pos:		     The current position in the iteration.
 * 
 * OUTPUT PARAMETERS:
 *		arg_count:	The number of arguments.
 *
 */
int tokenize_input_history(char *args[], char *history[], int pos){

	int count = 0;
	int arg_count = 0;
	
	if(history != NULL && pos >= 0){
		char *input = strdup(history[pos]);
		args[0] = strtok(input," ");

		while (args[count] != NULL) {
	
			count++;
			args[count] = strtok(NULL, " ");
			arg_count++;
		
		} 
	
		//did this to cut off any extra commands from a previous command
		//that was longer *this is very important
		count++;
		args[count] = '\0';
	}
	
	return arg_count;
	
}

/**
 * PURPOSE: Fork off a child to execute a command.
 * NOTE:    This was placed in it's own function otherwise code would be duplicated.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user or from history. These arguments 
 *					are to be executed by child.
 *
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void fork_off(char *args[]){

	//don't execute if it's a "set" command since we are the ones to process "set"
	if(args != NULL && args[0] != NULL && strcmp(args[0], "set") != 0){
		int pid;
		int rc;

		pid = fork();
		if(pid != 0){
			wait(NULL);
		}
		else{
			rc = execvp(args[0],args);
			if(rc != 0){
				printf("Invalid command\n");
			}
			exit(0);
		}
	}
	fflush(stdout);

}


int main(int argc, char *argv[]){

	char input[MAX_LENGTH];
	char *args[ARG_LENGTH];
	char *history[HISTORY_SIZE];
	int arg_count = 0;
	int hist_size = 0;
	int hist_position = 0;
	int i = 0;
	int ch;
    	int first;
    	int second;
	boolean up = false;
	boolean up_first = false;
	boolean iterating = false;
	int up_counter = 0;

	//an array of structs, this keeps a history of variables and their values
	Argument arguments[MAX_LENGTH];	
	int s_index = 0;
	int *ptr;
	ptr = &s_index;
	
	while(1){
	
		process_keypress();
		ch = getchar();
	   	first = second;
	  	second = ch;

		//up arrow key press
        	if(first == '[' && second == 'A'){
        	 	iterating = true;
        	 	up = true;
        	 	if(up_counter == 0){
        	 		up_first = true;
        	 	}
        	 	else{
        	 		up_first = false;
        	 	}
        	 	up_counter ++;
        	 	fflush(stdout);
        	 	hist_position = iterate_history(history, hist_size, hist_position, up, up_first);
				fflush(stdout);
        	}

			//down arrow key press
        	else if(first == '[' && second == 'B'){
        		iterating = true;
        		up = false;
        		up_first = false;
				hist_position = iterate_history(history, hist_size, hist_position, up, up_first);
				fflush(stdout);
       	}
       	else {
       		//echo only ascii printable characters
       		if(iterating == false && ch >= 32 && ch <= 127){
     			printf("%c", ch);
      		}
      		if(iterating == false && i < MAX_LENGTH && ch != '\n'){
				input[i] = ch;
				i++;
			}
		}

		//if we want to execute a command in history
		if(iterating == true && ch == '\n'){
			up_counter = 0;
			input[0] = '\0';
			i = 0;
			
			//store any commands in the history, dont' store empty commands
			if(history[hist_position] != NULL && strlen(history[hist_position]) > 0){

				if(hist_size < HISTORY_SIZE){
				 	history[hist_size] = strdup(history[hist_position]);
					hist_position = hist_size;
					hist_size ++;
					
				}
				//if history full, take out least recent and put in this one
				else{
					char *new_entry = strdup(history[hist_position]);
					shift_history(history, new_entry);
					hist_position = HISTORY_SIZE-1;
				}
				
			}
			
			arg_count = tokenize_input_history(args, history, hist_position);
			printf("\n");
			check_if_set_command(args, arg_count, arguments, ptr);
			fork_off(args);
			
			iterating = false;
			
		}
		//if we want to execute a command typed in
		if(iterating == false && ch == '\n'){
	
			up_counter = 0;
			printf("\n");
			input[i] = '\0';
			if(input[0] == '\n'){
				input[0] = '\0';
			}
			i  = 0;

			//store any commands in the history, dont' store empty commands
			if(strlen(input) > 0 && input[0] != '\0' && input[0] != '\n'){
				if(hist_size < HISTORY_SIZE){
				 	history[hist_size] = strdup(input);
					hist_position = hist_size;
					hist_size ++;
					
				}
				else{
					char *new_entry = strdup(input);
					shift_history(history, new_entry);
					hist_position = HISTORY_SIZE-1;
				}
				
			}
			arg_count = tokenize_input(args, input);
			check_if_set_command(args, arg_count, arguments, ptr);
			
			fork_off(args);
		
		}
		fflush(stdout);
	}

	
	
	
	
	
	
	
	
	
	
	
	
	return 0;

}
