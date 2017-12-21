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

int row, col,h_col; 					//polojenie cursor
int old_COLS,old_LINES;
int poscurinstr, curstr, levelinstr;
int beg, max_s;					//номер верхней строки на экране, TOTAL количество строк
int nostr,con_str, stronscr;			//количество строк в файле, признак продолжения считывания строки, количество строк текста на экрне
char **t_file=NULL;			//в ту же строку из нового буфера; массив строк текста
char **t_scr=NULL;			//for text on screen
char buf[BUFSIZE+1];		//буфер для чтения из файла
char namefile[30];			// имя файла для чтения или сохранения

void sig_winch(int signo);							//14 обработка прерывания
void scroll_s(WINDOW *win, int scroll);	//15 прокручивание экрана

void tscr_to_tfile();						//16
void tfile_to_tscr();						//17
void box_name_file(int ftype);	//18
int	 save_file(char *f_name);		//19
int	 read_file(char *f_name);		//20 чтение текста из файла
void convert_buf(long n);				//21 разбивка данных на строки

void *mem_to_list(void *point, long len);	//22
void *mem_to_str(long len);								//23
void clear_file(char** t);								//24
void show_msg(char *msg);				//25
void input_enter();							//26

void prog_ini();						//01
void edit_text();						//02
void print_file();					//03 вывод текста на экран в графическом режиме
void new_file();						//04

void prog_end();						//06

void updcoordcur();					//07
void mwcur(int direction);	//08
void del_ch(int direction);	//09
void input_ch(char c);			//10
void newstrdown(int position);	//11
void delemptystr(int nstr);	//12
void concat_str(int numb);	//13



int main(int argc, char **argv){
	nostr = con_str = 0;
	prog_ini();

	if (argc > 1) {
		strcpy(namefile,argv[1]);
		read_file(namefile);
		row = col =	beg = 0;
		print_file(0, max_s);
	}
	else
		new_file();//		read_file("7942.txt");
	
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

void print_file(int pos, int numb){//03
	for(int i=pos; i< pos+ numb +1; i++){
		if ((i > LINES -1) || (i + beg > max_s -1))
			break;
		mvaddstr(i, 0, t_scr[ beg + i ] );
	}	
	move(row, col);
	refresh();
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
//	mousemask(BUTTON1_CLICKED, NULL);
	refresh();
}


void mwcur(int direction){//08
	int endstr = strlen(t_scr[row+beg]);
	switch( direction ){
		case 0:							//KEY_UP:
			if (row>0){		// есть куда переместить курсор выше
				row--;
//				h_col=col;
 				if (endstr < h_col) 
					col=endstr-1;
			}
			else if (beg>0) {		// мы на верхней строке и вверху есть ещё строки  
				scroll_s(stdscr, 1); //1 rool up, 0 roll down
 				if (endstr < h_col) 
					col=endstr-1;
			}
			else if (col > 0 )
				h_col=col =  0;
			else flash();
			
			break;
		case 1:							//KEY_DOWN:
			if ((row<LINES-1) && (row + beg < max_s-1) ){ // есть куда переместить курсор ниже
				row++;
 				if (endstr < h_col) 
					col=endstr-1;
			}
			else if ( beg<(max_s-LINES) ){ 	// мы на нижней строке и снизу есть ещё строки 
				scroll_s(stdscr, 0);
 				if (endstr < h_col) 
					col=endstr-1;
			}
			else if (col < strlen(t_scr[row+beg]) )
				h_col=col =  strlen(t_scr[row+beg]);//MOVE CUR TO END OF STR IF IT FARAWAY
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
	move(row,col);
}

void del_ch(int direction){ //09
	int endstr = strlen(t_scr[row+beg]);
	int nrow=row, ncol;
	if (direction) { //если клавиша дел
		if (col == endstr ){	// если пытаемся удалить конец строки
			if (row +beg < max_s -1)  //если это не последняя строка текста
				concat_str(row+beg );						// объединяем со следующей строкой
			else flash(); //если последняя то мигаем
			nrow=-1;
		}
	}
	else // бэкспейс
			if (col==0){ // если пытаемся стереть начало строки
				if (row +beg > 0) //и выше есть строки
					concat_str(row+beg -1); //соединяем с предыдущей строкой
				else flash(); //выше строк нет мигаем
				nrow=-1;
			}
			else col--; //сдвигаем курсор на 1 символ влево
	if(nrow != -1) {
		ncol=col;
		nrow--;
		do{ //сдвиг всех нижележащих символов абзаца на 1 влево
			nrow++;
			endstr = strlen(t_scr[nrow+beg]);

			for(int i=ncol; i< endstr; i++) // переписываем символ последующим т.е. сдвигам строку 
				t_scr[nrow+beg][i]= t_scr[nrow+beg][i+1]; //на один шаг на место удаленного

			if( !t_scr[nrow+beg][COLS+1] ) {	// если  это не последняя строка в абзаце
				ncol = 0; //следущую итерацию начинаем с первого символа строки
				t_scr[nrow+beg][endstr-1] = t_scr[nrow+beg+1][0]; //копируем в конец строки первый 
			}																										//символ следующей строки
		}while( !t_scr[nrow+beg][COLS+1] ); //продолжаем пока это не последняя строка абзаца
		print_file(row, nrow - row ); // обновляем строки в которых произошли изменения
		//delch();	// удаляем символ на экране
	//upd_str_in_tfile (cur str, count, add/rem)
	//upd_str_in_tfile (row+beg, 1, 0);
	}
}

void input_ch(char c){//10
	int i,j,far,tofar,endstr;
	//t_scr[row+beg]
	far=row;
	while(!t_scr[far+beg][COLS+1] ) //подсчитываем сколько строк в абзаце
		far++;
	tofar = far;

	endstr = strlen(t_scr[far+beg]); 
	if (endstr == COLS) {						//если последняя строка абзаца заполнена
		newstrdown(far+beg); // добавляет пустую строку в указанном месте, ниже строки сдвигаются вниз
		t_scr[far+beg][COLS +1] = 0; // эта строка больше не конец абзаца
		far++;		
	}

	endstr = strlen(t_scr[far+beg]);
	for(i=far; i>row; i--){
		if(endstr< COLS )		////если строка неполная
			endstr++;				//увиличить диапазон копирования чтобы перемещался и символ \0
		else endstr--;		//иначе уменьшить чтобы последний символ не трогать(это ниже будет)
		for(j= endstr; j>0; j--)
			 t_scr[i +beg][j]= t_scr[i +beg][j-1]; //смещаем символы влево

		endstr = strlen(t_scr[i +beg-1]);		//вычисляем длину предстроки
		t_scr[i +beg][0]= t_scr[i +beg-1][endstr-1];	// копируем послений символ пред строки
	}																									// на перое место текущей
	endstr = strlen( t_scr[row+beg] );
	if(endstr< COLS )		////.... copy + \0
		endstr++;
	else endstr--;

	for(j= endstr; j>col; j--)		// сдвигаем символы текущей строки
		 t_scr[row+beg][j]= t_scr[row+beg][j-1];
	
	t_scr[row+beg][col]=c; // вставляем символ на экран и в строку

	if (far == tofar)
		print_file(row, far - row ); // обновляем строки в которых произошли изменения
	else
		print_file(row, LINES ); // обновляем строки до конца экрана
//	insch(c);
//	refresh();
//!!! upd cur str in t_file
	//upd_str_in_tfile (cur str, count, add/rem)
	//upd_str_in_tfile (row+beg, 1, 0);
}

void input_enter(){ //26
	int i,j,endstr,state_str;
	state_str = t_scr[row+beg][COLS +1];	//сохраняем признак конца текущей строки

	if (col > 0 ){ 	//если мы не в начале строки
		newstrdown(row+beg+1);			// создаём новую строку ниже

		endstr = strlen(t_scr[row+beg]);//получаем длину строки
		for(i=0; i < endstr - col; i++)	// копируем все символы после курсора на новую строку
			t_scr[row+beg+1][i] = t_scr[row+beg][col+i];
		t_scr[row+beg+1][i] = '\0'; 	// закрываем строку
		t_scr[row+beg][COLS +1] = 1; 	// старая строка теперь конец абзаца
		t_scr[row+beg][col] = '\0'; 	// устанавливаем конец строки там где был курсор
		if (state_str == 0)	//если строка в которую вставили перенос строки не была концом абзаца
			concat_str(row+beg+1);	//то склеиваем новую строку с нижележащей

		print_file(row, LINES); // обновляем строки до конца экрана
		col = 0;
		mwcur(1); // сдвигаем курсор вниз в начало, если необходимо экран прокручивается
	}
	else {		//если мы в начале строки
		if ( (row+beg > 0) && (t_scr[row+beg-1][COLS +1] == 0) ) // если мы не на первой строке текста
			 //и строка выше не является концом абзаца, то просто разделяем их и обновляем 2 строки
			t_scr[row+beg-1][COLS +1] = 1;
		else{ //иначе значит что мы или на верхней строке или выше конец абзаца
			newstrdown(row+beg);			// создаём пустую строку в этом месте, строки ниже сдвигаются вниз
			print_file(row, LINES); // обновляем строки до конца экрана
			mwcur(1); // сдвигаем курсор вниз на старую строку, если необходимо экран прокручивается
		}
	}
//!!! upd prev str and next in t_file, +1 new 
	//upd_str_in_tfile (cur str, count, add/rem)
	//upd_str_in_tfile (row+beg-1, 2, +1);
}

void newstrdown(int position){	//11 добавляет новую строку на экране в указанной позиции
	int i;		
	t_scr = (char**) mem_to_list(t_scr, ++max_s );	//добавляем ещё одну строку

	for(i=max_s-1; i> position; i--)	//сдвигаем строки вниз
		t_scr[i] = t_scr[i-1];

	t_scr[i] = (char*) mem_to_str( COLS +2); 	//на новом месте создаём строку
	strcpy(t_scr[i], "");
	t_scr[i][COLS +1] =1; 	//устанавливаем флаг конца строки
//  if(typenewstr)
//		tscr_to_tfile(); //upd text file in memory
}

void concat_str(int numb){ //13 объединение строки с нижележащей
	int endstrup, endstrdown,oldstrlen;
	int i,j,n,m;

	t_scr[numb][COLS+1] = 0;// убираем признак конца строки у строки к концу которой 
	endstrup = strlen(t_scr[numb]); //будем приклеивать другую строку 

	if ( endstrup < COLS ){ //проверяем есть ли свободное место в этой строке
		i=endstrup;	// end of prev str
		n=numb;			// numb of prev str
		j=0;				// first symbol next str
		m=numb+1;		// numb of next str
		endstrdown = strlen(t_scr[m]); 

		do {
			t_scr[n][i++] = t_scr[m][j++]; //копируем символы со следующей строки в предыдущую
			if (i > COLS-1 ){	//если строка кончилась переходим на начало следующей
				i=0;
				n++;
				//endstrup = strlen(t_scr[n]); 
			}
			if (j > endstrdown -1){
				j=0;
				m++;
				endstrdown = strlen(t_scr[m]); 
			}
		}while( (!t_scr[m][COLS+1] ) || (j < endstrdown) );
		t_scr[n][i] = '\0'; 	// закрываем строку
		t_scr[n][COLS+1] = 1;	// устанавливаем признак конца строки 

		if ( (endstrdown + endstrup) <= COLS ){ //если в сумме умещаются символы в одну строку
			delemptystr(m); // DEL LAST STR - удаляем последнюю строку
			print_file(numb, LINES); // обновляем строки до конца экрана
		}
		else 
			print_file(numb, m - numb); // обновляем строки этих абзацев
	}
//!!! upd cur str and next in t_file, -1 next 
	//upd_str_in_tfile (cur str, count, add/rem)
	//upd_str_in_tfile (numb, 2, -1)
}

void delemptystr(int nstr){ //12 удаление строки с данным номером в t_scr
	free(t_scr[nstr]);

	for(int i=nstr; i< max_s-1; i++)	//
		 t_scr[i] = t_scr[i+1];

	t_scr = (char**) mem_to_list(t_scr, --max_s );	//
}

void updcoordcur(){ //07 если изменилась ширина экрана, а высота вторична
	//col, row, beg		переразбиение строк если произошло изменение размеров окна
	int i,count=0;
	int nofstr,poscur,posscry,posscrx;

	if(old_COLS != COLS){	// если изменилась шинина экрана
		tscr_to_tfile();		//!!! 
		posscry=0;
		for(i=0; i<	beg; i++)		//находим какой номер строки текста в t_file 
			if ( t_scr[i][old_COLS+1] == 1) //соответствует верхней строке 
				posscry++;		//на экране

		for(i=beg-1; i>=0; i--)	//начинаем просматривать строки начиная с
			if ( t_scr[i][old_COLS+1] == 1) // предыдущей и ищем конец строки
				break;	// если находим то выходим, т.образом мы находим на какой 
		posscrx = old_COLS *( (beg-1) -i) ; //строке абзаца мы находимся.
		//далее находим отступ символа расположенного в л.в. углу экрана
		//от начала строки, номер которой найден выше

		//аналогично находится координаты положения курсора в строке t_file
		for(i=0; i<	beg+row; i++)
			if ( t_scr[i][old_COLS+1] == 1) 
				nofstr++;	
		for(i=beg+row-1; i>=0; i--)
			if ( t_scr[i][old_COLS+1] == 1) 
				break;
		poscur = old_COLS *( beg+row-1 -i) + col; 

		tfile_to_tscr(); // обновляем t_scr в соответствии с новой шириной экрана
	//pos of scr
		count=0;
		for(i=0; (i<max_s) && (count < posscry) ; i++) //перебираем строки
			if ( t_scr[i][COLS+1] == 1) 	//пока не найдем нужное количество
				count++;	//окончаний строк
		beg= i + posscrx / COLS;	// прибавим к данному числу количество полных
		if ( posscrx % COLS) 	//строк помещающихся на экране
			beg++;	//+1 если ещё текст остался

	//pos of cur
		count=0; 	// аналогично находим координаты курсора
		for(i=0; (i<max_s) && (count < nofstr) ; i++)
			if ( t_scr[i][COLS+1] == 1) 
				count++;
		row= i + poscur / COLS - beg;	// но отнимаем отступ строк на экране
		col= poscur % COLS;	// в результате может получится отрицательное число
		//или число превышающее LINES-1 это надо учесть при вводе текста и перемещении курсора
	}
	if(row>=LINES){ // (если курсор вышел за экран - передвинуть экран)
		beg += row - LINES+1;
		row = LINES-1;
	}		
	if(row<0){
		beg += row;
		row = 0;
	}		
	print_file(0, LINES); //обновить экран новым содержимым!!!!
}

void edit_text(){//02
	int loop=TRUE;
	int c;
	
	while(loop==TRUE){
		if ( (old_COLS != COLS) || (old_LINES != LINES) ){ //
			updcoordcur();			//save scr state /find old coord cur /calc new coord cursor on curr str /calc new state scr /refresh win
			h_col=col; 
			old_COLS = COLS;
			old_LINES = LINES;
		}

		c=getch();	//F3 openf  F2 savef   F4 exit
		if ( (KEY_F(2) != c) && (KEY_F(3) != c) && (KEY_F(4) != c) && (KEY_UP != c) && (KEY_DOWN != c) ) 
			h_col=col; //upd vrt coord cursor for stabile vert moovment

		if ( (31< c) && (c < 127) )
			input_ch(c);
		else{
			switch(c){
				case KEY_F(3): {
					box_name_file(0);	
					if (strlen(namefile)>0)						
						read_file(namefile);
						row = col =	beg = 0;
						print_file(0, LINES);
				break;}

				case KEY_F(2): {
					box_name_file(1);	
					if (strlen(namefile)>0) 
						save_file(namefile);
				break;}

				case KEY_F(4): loop=FALSE;	break;

				case KEY_UP: 		mwcur(0); break;
				case KEY_DOWN: 	mwcur(1); break;
				case KEY_LEFT: 	mwcur(2); break;
				case KEY_RIGHT:	mwcur(3); break;

				case KEY_BACKSPACE: del_ch(0);	break;
				case KEY_DL:				del_ch(1);	break;

				case '\n':			input_enter();	break;
			}
		}
	}
}

void prog_end(){ //06
	clear_file(t_file);
	clear_file(t_scr);
	endwin();	
}

void tfile_to_tscr(){//17
	int i,j,k,count;
	int nsubstr,lenstr;
	int cols=COLS;
	max_s =0;
	count=0;
	clear_file(t_scr);
	
	for(i=0; i<	nostr; i++){
		lenstr = strlen(t_file[i]);
		max_s += lenstr / cols;
		if ( lenstr % cols > 0)  //
			max_s++;
		if (lenstr == 0 ) 
			max_s++;
	}
	t_scr = (char**) mem_to_list(t_scr, max_s );

	for(i=0; i<	max_s; i++)
		 t_scr[i] = (char*) mem_to_str( cols +2); //

	for(i=0,k=0; i<	nostr; i++){
		
		lenstr= strlen(t_file[i]); 
		nsubstr= lenstr / cols;			//сколко целых подстрок в строке

		if( lenstr == 0){
			t_scr[count][0]=0; //0 - закрываем строку с нулевой длинной
			count++;
		}

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
		else
			t_scr[count-1][cols+1]=1; // если длина строки кратна экрану или пустая
	}
}

void tscr_to_tfile(){//16
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

void new_file(){ //04
	clear_file(t_file);
	nostr=1;

	t_file = (char**) mem_to_list(t_file, nostr);
	t_file[nostr-1] = (char*) mem_to_str( 1 );
	strcpy(t_file[nostr-1],"");	

	tfile_to_tscr();
	row = col =	beg = 0;	
}


void box_name_file(int ftype){//18
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

	if (ftype) //	1 - save 
		mvwaddstr(subwnd, 0, 0, namefile);

	//wattron(subwnd, A_BOLD);
//	curs_set(1);
//	keypad(stdscr, TRUE);
	echo();
	nocbreak();

	wrefresh(subwnd);
	wrefresh(wnd);

  mvwgetstr(subwnd, 0, 0, namefile);
	delwin(wnd);

	//clear();////////////////////////////
	noecho();
	cbreak();
}

void show_msg(char *msg){ //25
	WINDOW *wnd;
	int ctop,cleft,wwide;

	wwide=strlen(msg)+2;
	ctop= LINES/2;
	cleft= (COLS-wwide) /2;

	init_pair(1, COLOR_RED, COLOR_WHITE);
	wnd = newwin(1, wwide, ctop, cleft);
	wattron(wnd, COLOR_PAIR(1)|A_BOLD);

	mvwaddstr(wnd, 0, 1, msg);

	nodelay(wnd,FALSE);
	wrefresh(wnd);
	getch();
	nodelay(wnd,TRUE);

	delwin(wnd);
}

int save_file(char *f_name){ //19 добавить вывод сообщения об ошибке и чтобы программа не вылетала при этом
	char *entr="\n";
	char bufff[80];
	int my_f, err;
	ssize_t nr;
//	my_f = open(f_name, O_RDWR|O_NONBLOCK); //O_RDONLY O_WRONLY
	my_f = open(f_name, O_WRONLY);
	if (my_f == -1) {
		/*prog_end();
		perror ("open file to write");
		exit (EXIT_FAILURE); */
		err = errno;
		strcpy(bufff, "open file to write: ");
		strcat(bufff, strerror(err) );

		show_msg(bufff);
		return -1;
	}
	
	for(int i=0; i<nostr; i++){
		
		nr=write(my_f, t_file[i], strlen(t_file[i]) );
		if ( nr == -1 ){
/*			perror ("write to file");	
			prog_end();
			exit (EXIT_FAILURE);
*/			err = errno;
			strcpy(bufff, "write to file: ");
			strcat(bufff, strerror(err) );
			show_msg(bufff);
			return -1;
		}
		if ( nr != strlen(t_file[i]) ){
/*			perror ("write to file, something wrong ");	
			prog_end();
			exit (EXIT_FAILURE);
*/			err = errno;
			strcpy(bufff, "write to file, something wrong: ");
			strcat(bufff, strerror(err) );
			show_msg(bufff);
			return -1;
		}
		if( i < nostr-1){ //write to file "enter"
			nr=write(my_f, entr, strlen(entr) );
			if ( (nr == -1) || (nr == 0) ){
/*				perror ("write to file");	
				prog_end();
				exit (EXIT_FAILURE);
*/			err = errno;
			strcpy(bufff, "write to file: ");
			strcat(bufff, strerror(err) );
			show_msg(bufff);
			return -1;
			}
		}
	} 

	if(close(my_f) == -1){
		prog_end();
		perror ("close file");
		exit (EXIT_FAILURE);
	}
	return 0;
}

int read_file(char *f_name){ //20
	int my_f,err;
	char bufff[80];
	ssize_t nr;
//	my_f = open(f_name, O_RDWR|O_NONBLOCK); //O_RDONLY O_WRONLY
	my_f = open(f_name, O_RDONLY|O_NONBLOCK);
	if (my_f == -1) {
/*		perror ("open file");
		exit (EXIT_FAILURE);
*/	err = errno;
		strcpy(bufff, "open file: ");
		strcat(bufff, strerror(err) );
		show_msg(bufff);
		return -1;
	}
	clear_file(t_file);
	nostr=con_str=0;
	do{
 		nr=read(my_f, buf, BUFSIZE);
		if ( nr == -1 ){
			if ( (errno == EINTR ) || (errno == EAGAIN ) ){
				perror ("read file");	
				continue;
			}
			else {		
/*				perror ("read file");
				exit (EXIT_FAILURE);
*/			err = errno;
				strcpy(bufff, "read file: ");
				strcat(bufff, strerror(err) );
				show_msg(bufff);
				return -1;
			}
		}
		convert_buf(nr);
	} while (nr != 0);//EOF

	if(close(my_f) == -1){
		prog_end();
		perror ("close file");
		exit (EXIT_FAILURE);
	}
	return 0;
}

void convert_buf(long n){ //21
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
	while ( count ){			// работаем пока не обработаем все полученные данные 
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

	//функции работы с памятью

void clear_file(char** t){ //24
	for(int i=0; i<nostr; i++)
		free(t[i]);
	free(t);
}

void *mem_to_list(void *point, long len){ //22
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

void *mem_to_str(long len){ //23
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

void scroll_s(WINDOW *win, int scroll){ //15  1 rool up, 0 roll down
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

void sig_winch(int signo){ //14
	struct winsize size;
	ioctl(fileno(stdout), TIOCGWINSZ, (char *) &size);
	resizeterm(size.ws_row, size.ws_col);
	nodelay(stdscr, 1);
	while (wgetch(stdscr) != ERR );
	nodelay(stdscr, 0);
}
