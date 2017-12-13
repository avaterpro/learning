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

#define BUFSIZE 4000		//размер буфера для считывания файла

int row, col; 					//polojenie cursor
int old_COLS,old_LINES;
int poscurinstr, curstr, levelinstr;
int beg, max_s;					//номер верхней строки на экране, TOTAL количество строк
int nostr,con_str, stronscr;			//количество строк в файле, признак продолжения считывания строки, количество строк текста на экрне
char **t_file=NULL;			//в ту же строку из нового буфера; массив строк текста
char **t_scr=NULL;			//for text on screen
char buf[BUFSIZE+1];		//буфер для чтения из файла
char namefile[30];

void sig_winch(int signo);							//обработка прерывания
void scroll_s(WINDOW *win, int scroll);	//прокручивание экрана

void tscr_to_tfile();						//
void tfile_to_tscr();						//
void box_name_file(int ftype);	//
void save_file(char *f_name);		//
void read_file(char *f_name);		//чтение текста из файла
void convert_buf(long n);				//разбивка данных на строки

void *mem_to_list(void *point, long len);	//
void *mem_to_str(long len);								//

void prog_ini();						//
void edit_text();						//
void print_file();					//вывод текста на экран в графическом режиме
void new_file();						//
void clear_file(char** t);	//
void prog_end();						//

void mwcur(int direction);	//
void del_ch(int direction);	//

void input_ch(char c);					//
void newstrdown(int position, int typenewstr);	//
void delemptystr(int nstr);			//
void concat_str(int numb);			//



int main(int argc, char **argv){
	nostr = con_str = 0;

	if (argc > 1) {
		strcpy(namefile,argv[1]);
		read_file(namefile);
	}
	else
		new_file();//		read_file("7942.txt");
	
	prog_ini();
//	getch();	
	edit_text();

/*
	//print_file(stdscr);

	mousemask(BUTTON1_CLICKED, NULL);
	while (wgetch(stdscr) == KEY_MOUSE ){
		MEVENT event;
		getmouse(&event);
		move(0, 0);
		printw("Mouse button pressed at %i, %i\n", event.x, event.y);
		refresh();
		move(event.y,event.x);
	}
*/
	prog_end();
}

void print_file(WINDOW *win){
//int row, col; 					//polojenie cursor
//int poscurinstr, curstr, levelinstr;
//int beg, max_s;					//номер верхней строки на экране, TOTAL количество строк
//int nostr,con_str;			//количество строк в файле, признак продолжения считывания строки
	char *subs[200]; //symbols in line in terminal
	wmove(win, 0, 0);

	for(int i=0; i<nostr; i++){
		wprintw(win, "%s\n");
	}	
}

void prog_ini(){
	initscr();
	signal(SIGWINCH, sig_winch); 		//установка прерывания
//	scrollok(stdscr, TRUE); //razreshit prokrutku
	keypad(stdscr, TRUE);
	noecho();
//	notimeout(stdscr, FALSE);
//	nodelay(stdscr, TRUE); // getch not waiting press key
//	timeout(100);
	cbreak(); //
	start_color();
	old_COLS= COLS;
	old_LINES= LINES;
//
	mousemask(BUTTON1_CLICKED, NULL);
	refresh();
}


void mwcur(int direction){
	switch( direction ){
		case 0:							//KEY_UP:
			if (row>0) row--;		// есть куда переместить курсор выше
			else if (beg>0) 		// мы на верхней строке и вверху есть ещё строки  
				scroll_s(stdscr, 1); //1 rool up, 0 roll down
			else if (col > 0 )
				col =  0;
			else flash();
			
			break;
		case 1:							//KEY_DOWN:
			if ((row<LINES-1) && (row + beg < max_s-1) ) row++; // есть куда переместить курсор ниже
			else if ( beg<(max_s-LINES) ) 	// мы на нижней строке и снизу есть ещё строки 
				scroll_s(stdscr, 0);
			else if (col < strlen(t_scr[row+beg]) )
				col =  strlen(t_scr[row+beg]);//MOVE CUR TO END OF STR IF IT FARAWAY
			else flash();
			break;
		case 2:							//KEY_LEFT:
			if (col > 0) col--;	// есть куда переместить курсор влево
			else if (row > 0) {	// перемещать некуда, но (мы в тельняшках) выше есть строки
				row--;												//переходим на предыдущую строку в конец строки
				col= strlen(t_scr[row+beg]);
			}
			else if (beg > 0) { //слева места нет, мы на верхней строке экрана, но сверху есть ещё строки
				scroll_s(stdscr, 1);					//прокручиваем экран вверх
				col= strlen(t_scr[row+beg]); 	//переходим в конец строки
			}
			else flash();
			break;
		case 3:	;						//KEY_RIGHT:
			int endstr = strlen(t_scr[row+beg]);
			if (col < endstr) 
				col++;		//мы не дошли до конца строки
			else if ( (row < LINES-1) && (row + beg < max_s-1 ) ) {			//дошли до конца строки и ниже, на экране, есть строки И мы не на последней строке в тексте
				row++;
				col= 0;
			}
			else if ( beg<(max_s-LINES) ) { //мы на последней строке на экране и ниже есть еще строки
				scroll_s(stdscr, 1);// матаем ниже
				col= 0;							//курсор в начало стрки
			}
			else flash();
			break;
	}
}

void upd_ram(){

}

void del_ch(int direction){
	int endstr = strlen(t_scr[row+beg]);
	int nrow=row, ncol;
	if (direction) {
		if (col == endstr )
			if (row +beg < max_s -1)  // если пытаемся удалить конец строки
				concat_str(row+beg );						// i eto ne pos str	
			else flash();
	}
	else if (col==0){ // если пытаемся стереть начало строки
				 if (row +beg > 0) 
						concat_str(row+beg -1); 
				 else flash();
			 }
			 else col--;

	ncol=col;
	nrow--;
	do{
		nrow++;
		endstr = strlen(t_scr[nrow+beg]);

		for(int i=ncol; i<= endstr; i++) // переписываем символ последующим т.е. сдвигам строку 
			t_scr[nrow+beg][i]= t_scr[nrow+beg][i+1]; //на один шаг на место удаленного

		if( !t_scr[nrow+beg][COLS+1] ) {	// если  это не последняя строка в абзаце
			ncol = 0;
			t_scr[nrow+beg][endstr-1] = t_scr[nrow+beg+1][0]; //копируем в конец строки первый 
		}																										//символ следующей строки
	}while( !t_scr[nrow+beg][COLS+1] );
	delch();	// удаляем символ на экране
	refresh();
}

void input_ch(char c){
	int i,j,far,endstr;
	//t_scr[row+beg]
	far=row;
	while(!t_scr[far+beg][COLS+1] ) //подсчитываем сколько строк в абзаце
		far++;

	endstr = strlen(t_scr[far+beg]); 
	if (endstr == COLS) {						//если последняя строка абзаца заполнена
		newstrdown(far+beg, 0); // добавляет пустую строку в указанном месте, ниже строки 
														// сдвигаются вниз, вызывает изменения в t_scr //и t_f 
		t_scr[far+beg][COLS +1] = 0; // эта строка больше не конец абзаца
		far++;		
	}

	endstr = strlen(t_scr[far+beg]);
	for(i=far; i>row; i--){

		if(endstr< COLS )		////если строка неполная
			endstr++;				//увиличить диапазон копирования чтобы перемещался и символ \0
		else endstr--;		//иначе уменьшить чтобы последний символ не трогать(это ниже будет)
		for(j= endstr; j>0; j--)
			 t_scr[i +beg][j]= t_scr[i +beg][j-1]; //перемещаем символы влево

		endstr = strlen(t_scr[i +beg-1]);		//вычисляем длину пред строки
		t_scr[i +beg][0]= t_scr[i +beg-1][endstr-1];	// копируем послений символ пред строки
	}																									// на перое место текущей
	endstr = strlen( t_scr[row+beg] );
	if(endstr< COLS )		////.... copy + \0
		endstr++;
	else endstr--;

	for(j= endstr; j>col; j--)		// сдвигаем символы текущей строки
		 t_scr[row+beg][j]= t_scr[row+beg][j-1];
	
	t_scr[row+beg][col]=c; // вставляем символ на экран и в строку
	insch(c);
//pud cur str in t_file
}

void newstrdown(int position, int typenewstr){	//добавляет новую строку на экране
	int i;		//ниже указанной позиции, 2й параметр (0,1) 1-если новый абзац, 0-внутри абзаца
	t_scr = (char**) mem_to_list(t_scr, ++max_s );	//

	for(i=max_s-1; i> position; i--)	//
		t_scr[i] = t_scr[i-1];

	t_scr[i] = (char*) mem_to_str( COLS +2); 	//
	strcpy(t_scr[i], "");
	t_scr[i][COLS +1] =1;
  if(typenewstr)
		tscr_to_tfile(); //upd text file in memory
}

void delemptystr(int nstr){
	int i;

	if (!t_scr[nstr-1][COLS+1]) 	//если вышележащая строка не является концом абзаца
		t_scr[nstr-1][COLS+1]=1;		//она им становится

	free(t_scr[nstr]);

	for(i=nstr; i< max_s-1; i++)	//
		 t_scr[i] = t_scr[i+1];

	t_scr = (char**) mem_to_list(t_scr, --max_s );	//
	
	tscr_to_tfile(); //upd text file in memory
}

void concat_str(int numb){
	int endstrup, endstrdown,oldstrlen;
	int i,j,n,m;

	t_scr[numb][COLS+1] = 0;
	oldstrlen = endstrup = strlen(t_scr[numb]); 

	if ( endstrup < COLS ){
		i=endstrup;	// end of prev str
		n=numb;			// numb of prev str
		j=0;				// first symbol next str
		m=numb+1;		// numb of next str
		
		do {
			t_scr[n][i++] = t_scr[m][j++];
			if (i >= COLS ){
				i=0;
				n++;
				endstrup = strlen(t_scr[n]); 
			}
			if (j >= COLS ){
				j=0;
				m++;
				endstrdown = strlen(t_scr[m]); 
			}
		}while( (!t_scr[m][COLS+1] ) || (j < endstrdown) );

//		for( i=

		if ( (oldstrlen + endstrdown) <= COLS ) /// DEL LAST STR
			delemptystr(m);
	}

}

void edit_text(){
	int loop=TRUE;
	int c;
	
	while(loop==TRUE){
		c=getch();	//F3 openf  F2 savef   F4 exit
		switch(c){
			case KEY_F(3): box_name_file(0);	/*read_file(namefile);*/	break;
			case KEY_F(2): box_name_file(1);	/* if (strlen(namefile)>0) save_file(namefile);*/	break;
			case KEY_F(4): loop=FALSE;	break;

			case KEY_UP: 		mwcur(0); break;
			case KEY_DOWN: 	mwcur(1); break;
			case KEY_LEFT: 	mwcur(2); break;
			case KEY_RIGHT:	mwcur(3); break;

			case KEY_BACKSPACE: del_ch(0);	break;
			case KEY_DL:				del_ch(1);	break;

/*			case: 	break;
			case: 	break;
			case: 	break;
			case: 	break;
			default: c++;		*/
		}
	}
}

void prog_end(){
	clear_file(t_file);
	clear_file(t_scr);
	endwin();	
}

void scroll_s(WINDOW *win, int scroll){ //1 rool up, 0 roll down
   /* пробуем, должны ли мы прокрутить вниз и если что-нибудь есть,
    то прокрутить*/
   if((scroll==0)&&(beg<(max_s-LINES))){  // beg nomer stroki na ekrane, max_s numb of str
      beg++;    						/* одна строка вниз */
      scrollok(win, TRUE);  /* задаем права на прокрутку */
      wscrl(win, +1);    		/* прокручиваем */
      scrollok(win, FALSE); /* отклоняем права на прокрутку */

   /* устанавливаем новую строку в последней линии */
      mvwaddstr(win,LINES-1,0,t_scr[beg+LINES-1]);  // 17 numb of lines
   /* очищаем последнюю строку от последнего символа до конца
    * строки. Иначе атрибуты не будут учтены.    */
       wclrtoeol(win); // 66 numb of |||
   }
   else if((scroll==1)&&(beg>0)){
      beg--;
      scrollok(win, TRUE);
      wscrl(win, -1);
      scrollok(win, FALSE);
      mvwaddstr(win,0,0,t_scr[beg]);
      wclrtoeol(win);
   }
   wrefresh(win);
return;
}

void add_ch_scr(){
/*
int row, col; 					//polojenie cursor
int poscurinstr, 
curstr, 
levelinstr;

int beg, max_s;					//номер верхней строки на экране, TOTAL количество строк

int stronscr;			// количество строк текста на экрне
char **t_file=NULL;			// массив строк текста
char **t_scr=NULL;			//for text on screen
*/
	
}

void tfile_to_tscr(){
	int i,j,k,count;
	int nsubstr,lenstr;
	int cols=COLS;
	max_s =0;
	count=0;
	clear_file(t_scr);
	
	for(i=0; i<	nostr; i++){
		max_s += strlen(t_file[i]) / cols;
		if (strlen(t_file[i]) % cols > 0)  //
			max_s++;
	}
	t_scr = (char**) mem_to_list(t_scr, max_s );

	for(i=0; i<	max_s; i++)
		 t_scr[i] = (char*) mem_to_str( cols +2); //

	for(i=0,k=0; i<	nostr; i++){
		
		lenstr= strlen(t_file[i]); 
		nsubstr= lenstr / cols;			//сколко целых подстрок в строке

		for(j=0;j< nsubstr; j++){
			for(k=0;k<cols;k++)
				t_scr[count][k]= t_file[i][j*cols+k];
			t_scr[count][cols]=t_scr[count][cols+1]=0; //0 - признак продолжения строки
			count++;
		}

		if (lenstr % cols > 0){			//копирование остатков подстроки
			for(k= 0; k<lenstr - j*cols; k++)
				t_scr[count][k]=t_file[i][j*cols+k];
			t_scr[count][k]=0;	t_scr[count][cols+1]=1;	// 1 - конец строки
			count++;
		}
	}

}

void tscr_to_tfile(){
	char c[BUFSIZE+1];
	int i,count=0;

	clear_file(t_file);
	nostr=0;	

	for(i=0; i<	max_s; i++)
		if ( t_scr[i][old_COLS+1] == 1) 
			nostr++; 

	t_file = (char**) mem_to_list(t_file, nostr);

	strcpy(c,"");	
	for(i=0; i<	max_s; i++){
		strcat(c, t_scr[i]);
		if ( t_scr[i][old_COLS+1]==1){
			t_file[count] = (char*) mem_to_str( strlen(c) +1);
			strcpy(t_file[count++], c);
			strcpy(c,"");	
		}
	}
}

void box_name_file(int ftype){
	WINDOW *wnd;
	WINDOW *subwnd;
//	WINDOW *oldstd;
	char *str_o="Please, enter name of file to open", 
			 *str_s="Please, enter name of file to save";
	int ctop,cleft,wwide;
	wwide=42; //because
	ctop= LINES/2 -1;
	cleft= (COLS-wwide) /2;

	init_pair(1, COLOR_BLACK, COLOR_GREEN);
	init_pair(2, COLOR_WHITE, COLOR_BLUE);
//	getch();
	wnd = newwin(3, wwide, ctop, cleft);
	wattron(wnd, COLOR_PAIR(1)|A_BOLD);
	box(wnd, '|', '-');
	if (ftype) //0 - open	1 - save 
		mvwaddstr(wnd, 0, 4, str_s);
	else
		mvwaddstr(wnd, 0, 4, str_o);

	subwnd= derwin(wnd, 1, wwide-2, 1, 1);
	wbkgd(subwnd, COLOR_PAIR(2));
	//wattron(subwnd, A_BOLD);
//	curs_set(1);
//	keypad(stdscr, TRUE);
	echo();
	nocbreak();

	wrefresh(subwnd);
	wrefresh(wnd);

  mvwgetstr(subwnd, 0, 0, namefile);
	delwin(wnd);

	clear();////////////////////////////
	noecho();
	cbreak();
}

void save_file(char *f_name){
	char *entr="\n";
	int my_f;
	ssize_t nr;
//	my_f = open(f_name, O_RDWR|O_NONBLOCK); //O_RDONLY O_WRONLY
	my_f = open(f_name, O_WRONLY);
	if (my_f == -1) {
		prog_end();
		perror ("open file to write");
		exit (EXIT_FAILURE);
	}
	
	for(int i=0; i<nostr; i++){
		
		nr=write(my_f, t_file[i], strlen(t_file[i]) );
		if ( nr == -1 ){
			perror ("write to file");	
			prog_end();
			exit (EXIT_FAILURE);
		}
		if ( nr != strlen(t_file[i]) ){
			perror ("write to file, something wrong ");	
			prog_end();
			exit (EXIT_FAILURE);
		}
		if( i < nostr-1){ //write to file "enter"
			nr=write(my_f, entr, strlen(entr) );
			if ( (nr == -1) || (nr == 0) ){
				perror ("write to file");	
				prog_end();
				exit (EXIT_FAILURE);
			}
		}
	} 

	if(close(my_f) == -1)
		perror ("close file");
}

void read_file(char *f_name){
	int my_f;
	ssize_t nr;
//	my_f = open(f_name, O_RDWR|O_NONBLOCK); //O_RDONLY O_WRONLY
	my_f = open(f_name, O_RDONLY|O_NONBLOCK);
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

	if(close(my_f) == -1)
		perror ("close file");
}

void convert_buf(long n){
	long i,j,previ,ls,lf,count;
	char c[BUFSIZE+1],old_s[BUFSIZE+1]; 
	i=previ=0;
	ls=lf=0;
	count=n;
	strcpy(old_s,"");	

	if ( (con_str) && (n>0) ) {		// если при предыдущем считывании мы не нашли перенос строки
		strcpy(old_s, t_file[nostr-1]); // коприруем в буфер старую строку
		con_str=0;											//сбрасываем флаг переноса строки
	}
	while ( count ){			// работаем пока не прочитаем все полученные данные 
		lf=strlen(old_s);		// вычисляем длину старой строки, если не было переноса то ноль
		for(;((buf[i] != 13) && (buf[i] != 10) ) && i<n; i++); //ищем конец строки или до конца

		for(j=0;j<i-previ;j++)		//считаем разницу от предыдущего конца строки до текущего
			c[j]=buf[previ+j];			//записываем в строку
		c[j]='\0';								//закрываем ее
		ls=j+1;										//размер места для хранения строки
		previ=++i;								//записываем индекс положения конца строки

		if ( (buf[previ-1] == 13)&&(buf[previ] == 10) ){	//проверяем на /13/10
			previ=++i;		//сдвигаем индекс
			count--;			//отнимает 1 у счётчика обработаннных данных
		}
		if (i>=n){			//если прочитали весь буфер а конец строки не нашли
			con_str=1;		// флаг продолжения строки
			count=0;			// "Спектаклей больше не будет!"
		}
		else
			count-=ls; 		// уменьшаем счетчик обработанных данных

		if (lf) 						//если был перенос
			free(t_file[nostr-1]);// освобожаем старую строку (потом новую запросим)
		else{
			nostr++;					//переноса не было - увеличиваем количество строк
			t_file = (char**) mem_to_list(t_file, nostr);
		}										// запрашиваем память для хранения указателя на новую строку

		if (lf+ls > BUFSIZE+1){ //размер суммы строк больше текстовой переменной 
			fprintf(stderr, "BUFSIZE too small");
			exit (EXIT_FAILURE);  // ай-яй-яй
		}

		strcat(old_s,c); //совмещаем строки, если переноса не было то получается новая строка
		t_file[nostr-1] = (char*) mem_to_str( strlen(old_s)+1 ); 	//выделяем память под строку
		strcpy(t_file[nostr-1],old_s); //записываем строку в список с строками файла
//		printf("%3ld %3ld s:%4d \'%s\'\n", lf, ls, nostr, old_s);
		strcpy(old_s,"");		//очищаем
	}
}

void clear_file(char** t){
	for(int i=0; i<nostr; i++)
		free(t[i]);
	free(t);
}

void new_file(){
	clear_file(t_file);
	nostr=1;

	t_file = (char**) mem_to_list(t_file, nostr);
	t_file[nostr-1] = (char*) mem_to_str( 1 );
	strcpy(t_file[nostr-1],"");	

	tfile_to_tscr();
}

void *mem_to_list(void *point, long len){
	void *p;
	p = realloc (point, len*sizeof(char*));
	// запрашиваем память для хранения указателя на новую строку
	if (p == NULL){ //
		printf("ERROR: %s", strerror(errno));
		// fun_emergency_exit
		exit (EXIT_FAILURE);  // ай-яй-яй
	}
	else 
		return p;
}

void *mem_to_str(long len){
	void *p;
	p = malloc ( len * sizeof(char) );
	//выделяем память под строку
	if( p ==NULL)	{ //
		printf("ERROR: %s", strerror(errno));
		// fun_emergency_exit
		exit (EXIT_FAILURE);  // ай-яй-яй
	}
	else 
		return p;
}

void sig_winch(int signo){
	struct winsize size;
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
	resizeterm(size.ws_row, size.ws_col);
	nodelay(stdscr, 1);
	while (wgetch(stdscr) != ERR );
	nodelay(stdscr, 0);
}

