#include <termios.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 250

int beg, max_s;
int nostr,con_str;
char **t_file=NULL;
char buf[BUFSIZE+1];
char c[BUFSIZE+1],old_s[BUFSIZE+1];

void sig_winch(int signo);
void scroll_s(WINDOW *win, int scroll);
void read_file();
void convert_buf(long n);
void print_file();


int main(int argc, char **argv){
	nostr = con_str = 0;

	read_file();
	getch();
	print_file(stdscr);
	refresh();
	getch();

/*
	initscr();
	signal(SIGWINCH, sig_winch);
	scrollok(stdscr, TRUE); //razreshit prokrutku
//	keypad(stdscr, TRUE);
	move(0, 0);

	mousemask(BUTTON1_CLICKED, NULL);
	move(2, 2);
	printw("Test your mouse...\n");
	printw("Or press any key to quit...\n");
//	cbreak();
//	noecho();
	while (wgetch(stdscr) == KEY_MOUSE ){
		MEVENT event;
		getmouse(&event);
		move(0, 0);
		printw("Mouse button pressed at %i, %i\n", event.x, event.y);
		refresh();
		move(event.y,event.x);
	}
	endwin();
	exit(EXIT_SUCCESS);
*/
}

void print_file(WINDOW *win){
	wmove(win, 0, 0);

	for(int i=0; i<nostr; i++){
		wprintw(win, "%s\n");
	}	
}


void convert_buf(long n){
	long i,j,previ,ls,lf,count;
	char *temp;
	i=previ=0;
	ls=lf=0;
	count=n;

	strcpy(old_s,"");	

	if (con_str) {		//con_str=0;// if was need continue string
		lf = strlen(t_file[nostr-1]);
		strcpy(old_s, t_file[nostr-1]);
		printf("N O STR: %d OLD S \'%s\'\n", nostr, old_s);
		con_str=0;
	}


	while ( count ){

		if (!con_str) {
			nostr++;
			t_file = (char**) realloc (t_file, nostr*sizeof(char*));
		}

		for(;buf[i] != '\n' && i<n; i++);

		for(j=0;j<i-previ;j++)
			c[j]=buf[previ+j];
		c[j]='\0';
		ls=j+1;
		previ=++i;

		if(nostr==20) 
			printf("!!!\'%s\'!!!\'%s\' :%3ld=%3d \n",c, old_s, ls, strlen(c) );

		if (i>=n){
			con_str=1;
			count=0;
		}
		else
			count-=ls;

		printf("NOSTR:%3d; COUNT:%3ld N:%ld LS:%3ld I:%3ld CSTR:%d NEW S\'%s\'\n", nostr, count, n, ls, i, con_str, c);

		if (lf+ls > BUFSIZE+1){
			fprintf(stderr, "BUFSIZE too small");
			exit (EXIT_FAILURE);
		}
		strcat(old_s,c);

		if(nostr==20) 
			strcpy(old_s,"");//printf("!!!\'%s\' :%3ld=%3d \n",old_s, lf, strlen(old_s) );

printf("23\n");
		if( (temp = (char*) realloc (t_file[nostr-1], (strlen(old_s)+1)* sizeof(char)) ) ==NULL)
			printf("ERROR: %s", strerror(errno));
		else
			t_file[nostr-1]=temp; 
printf("24\n");
			
//    perror ("Add mem for str");
		strcpy(t_file[nostr-1],old_s);
		strcpy(old_s,"");

	}
printf("33\n");

}

void scroll_s(WINDOW *win, int scroll){ //1 rool up, 0 roll down
   /* пробуем, должны ли мы прокрутить вниз и если что-нибудь есть,
    то прокрутить*/
   if((scroll==0)&&(beg<=(max_s-LINES))){  // beg nomer stroki na ekrane, max_s numb of str
   /* одна строка вниз */
      beg++;
   /* задаем права на прокрутку */
      scrollok(win, TRUE);
   /* прокручиваем */
      wscrl(win, +1);
   /* отклоняем права на прокрутку */
      scrollok(win, FALSE);
   /* устанавливаем новую строку в последней линии */
      mvwaddnstr(win,LINES-1,0,t_file[beg+LINES-1],-1);  // 17 numb of lines
   /* очищаем последнюю строку от последнего символа до конца
    * строки. Иначе атрибуты не будут учтены.
    */
       wclrtoeol(win); // 66 numb of |||
   }
   else if((scroll==1)&&(beg>0)){
      beg--;
      scrollok(win, TRUE);
      wscrl(win, -1);
      scrollok(win, FALSE);
      mvwaddnstr(win,0,0,t_file[beg],-1);
      wclrtoeol(win);
   }
   wrefresh(win);
return;
}


void read_file(){
	int my_f;
	ssize_t nr;

	my_f = open("testf.txt", O_RDWR|O_NONBLOCK);
	if (my_f == -1) {
		perror ("open file");
		exit (EXIT_FAILURE);
	}
	do{
 		nr=read(my_f, buf, BUFSIZE);
		if ( nr == -1 ){
			if ( (errno == EINTR ) || (errno == EAGAIN ) ){
				perror ("read file");	
				continue;
			}
			else {		
				perror ("read file");
				exit (EXIT_FAILURE);
			}
		}
		convert_buf(nr);
	} while (nr != 0);//EOF
	close(my_f);
	
	printw("read %d strings \n", nostr);
	refresh();
	
	for(int i=0; i<nostr; i++){
		//calculate num of line //maybe
	}
}


void sig_winch(int signo){
	struct winsize size;
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
	resizeterm(size.ws_row, size.ws_col);
	nodelay(stdscr, 1);
	while (wgetch(stdscr) != ERR );
	nodelay(stdscr, 0);
}

