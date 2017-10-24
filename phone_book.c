#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define N_OFCHAR 64+1

typedef struct Member {
	int id;
	char *name;
	char *phone;
} Member;

int add_usr();
int find_usr();
int print_all();

Member *members=NULL;
int number_of_members=0;

int main(){

	int i,flag;
	char c;
	
	//1. add usr
	//2. find by phone
	//3. print all list
	//4. quit
	flag=1;
	do{ 
		printf("\nPhone book\n 1. add usr\n 2. find by phone\n 3. print all list\n 4. quit\n");
		scanf("%c",&c);
		switch (c) {
			case '1': add_usr();  		break;
			case '2': find_usr(); 		break;
			case '3': print_all(); 		break;
			case '4': flag=0;  				break;
		}	
	} while (flag);

	for(i=0;i<number_of_members;i++){
			free(members[i].name);
			free(members[i].phone);
	}
	free(members);

	return 0;
}

int add_usr(){
	char buf[30];
	int i=++number_of_members;

	members = (Member*) realloc(members, i * sizeof(Member));
	members[i-1].id=i;

	printf("Adding new record.\nEnter name: ");
	scanf("%s",buf);
	members[i-1].name = (char*) malloc(strlen(buf) + 1);
	strcpy(members[i-1].name,buf);

	printf("Enter phone: ");
	scanf("%s",buf);
	members[i-1].phone = (char*) malloc(strlen(buf) + 1);
	strcpy(members[i-1].phone,buf);
}

int find_usr(){
	char buf[30];
	int i,count_find=0;
	printf("Search of record by the number.\n Enter number to find:");
	scanf("%s",buf);
	
	for(i=0;i<number_of_members;i++)
		if( strstr(members[i].number,buf) != NULL ) {
			if( count_find++ == 0 )
				printf("Found records:\n");
			printf("Record %d name: \"%s\" number: \"%s\"\n", 
							members[i].id, members[i].name,members[i].phone);
		}
	if (count_find > 0) 
		printf("Total found: %d records\n", count_find);
	else
		printf("No records found\n");
}

int print_all(){
	printf("Print all records:\n");
	
	for(int i=0;i<number_of_members;i++)
		printf("Record %d name: \"%s\" number: \"%s\"\n", 
				members[i].id, members[i].name,members[i].phone);
}
