#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printErrorPage(){
	printf("Content-Type:text/html\n\n");
	printf("<html>\n<header><title>Error!</title></header>\n");
	printf("<body><h1>Username and Password not found!</h1><p>Please try again:</p>");
	printf("<a href=\"login.html\"> login </a> <br /> <a href=\"index.html\"> home </a></body></html>");
}

typedef struct MEMBER {
	char fullname[30];
	char username[30];
	char password[30];
} member;


csvParser(char* line, member* user){
	const char d[2] = ",";
	char* temp;

	temp = strtok(line, d);
	strcpy(user->fullname, temp);
	temp = strtok(NULL, d);
	strcpy(user->username, temp);
	temp = strtok(NULL, d);
	*strchr(temp,'\n')='\0';
	strcpy(user->password, temp);
}

int main(int argc, char* argv[]){
	FILE* csv; //file pointer
	char csv_line[255]; //temporary file_line string
	member* current; //stores file_line string in a struct
	int content_length=255;	

	//Get query string
	if (content_length == 0) return 0;
	char* login = malloc(content_length+1);
	if (fgets(login, content_length, stdin) == NULL) return 0;

	//Parse query string
	member* user = malloc(sizeof(member));
	char* temp;
	temp = strtok(login, "&");
	login=strstr(temp, "=");
	strcpy(user->username,++login);
	temp = strtok(NULL, "&");
	login=strstr(temp, "=");
	strcpy(user->password,++login);	
	*strchr(user->password,'\n')='\0';		

	//Open CSV file
	csv = fopen("members.csv", "r");
	if (csv == NULL) return 0;

	while(fgets(csv_line,255,csv) != NULL){
		if ((current = (member*) malloc(sizeof(member)))==NULL) return 0;
		csvParser(csv_line, current); //deliminate and store in struct
		//If usernames match, break.
		if (strcmp(user->username,current->username) == 0) break;
		//Else, free current member and try next line
		else{
			free(current);
			current=NULL;
		}
	}

	//If username was not found, then return error
	if (current==NULL){
		printErrorPage();
		return 0;
	}	
	//If passwords do not match, then return error	
	if (strcmp(user->password,current->password) != 0){
		printErrorPage();
		return 0;
	}

	//Append to loggedin.csv
	csv = fopen("loggedin.csv","a");
	if (csv == NULL) return 0;
	strcat(user->username,"\n");
	fputs(user->username,csv);

	printf("Content-Type:text/html\n\n");

char raw[0x10], cookie[0x40] = { 0 };
FILE *urand;
int i;

urand = fopen("/dev/urandom", "rb");
fread(raw, sizeof(raw), 1, urand);
fclose(urand);

for (i = 0; i != sizeof(raw); ++i)
	sprintf(cookie + i * 2, "%02X", raw[i]);

printf("Set-Cookie: session=%s", cookie);

	printf("<html><head><meta http-equiv=\"refresh\" content=\"0; url=\"cs.mcgill.ca/~rberna14/comp206-a4/catalog.py\" /></head></html>");
}

