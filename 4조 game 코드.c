#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <asm/fcntl.h>
#include <string.h>
#include <termios.h>

#define FPGA_LED	0x12400000
#define FPGA_CLCD_WR    0x12300000  //command
#define FPGA_CLCD_RS    0x12380000  //data
#define KEY_OUT_ADDR   0x11D00000			
#define KEY_IN_ADDR    0x11E00000
#define FND_CS0        0x11000000
#define SCAN_NUM       4
#define FPGA_DOT_COL1 0x11800000
#define FPGA_DOT_COL2 0x11900000
#define FPGA_DOT_COL3 0x11A00000
#define FPGA_DOT_COL4 0x11B00000
#define FPGA_DOT_COL5 0x11C00000

#define FND_CS0 0x11000000
#define FND_CS1 0x11100000
#define FND_CS2 0x11200000
#define FND_CS3 0x11300000
#define FND_CS4 0x11400000
#define FND_CS5 0x11500000
#define FND_CS6 0x11600000
#define FND_CS7 0x11700000

static void clear_print(void);
static void end_print(void);
static struct termios initial_settings, new_settings;
static void score(void);
static void setcommand(unsigned short command);
static void initialize_clcd(void);
static void function_set(int DL, int N, int F);
static void display_control(int D, int C, int B);
static void cursor_shift(int set_screen, int set_rightshift);
static void entry_mode_set(int ID, int S);
static void return_home(void);
static void clcd_clear(void);
static      set_RAM_address(int pos, int CG_or_DD);
static void clcd_exit(void);
void        write_byte(char ch);
static void CG_clear(void);
static void spawn(int monster1, int position);
static void create(unsigned int* character);
unsigned short *CLCD_CMD, *CLCD_DATA;
static int level_normal(int level);
static void level_end(int level);
int dot_level=1;
static int peek_character=-1;
unsigned short *FND0, *FND1, *FND2, *FND3, *FND4, *FND5, *FND6, *FND7;
unsigned short *LED;
unsigned short *dot;
unsigned int fd0;
unsigned short *DOT_COL1, *DOT_COL2, *DOT_COL3, *DOT_COL4, *DOT_COL5;

unsigned short dot_table[10][5] = {
	{0x00, 0x00, 0x7F, 0x00, 0x00},
	{0x4F, 0x49, 0x49, 0x49, 0x79},
	{0x49, 0x49, 0x49, 0x49, 0x7F},
	{0x78, 0x08, 0x08, 0x7F, 0x08},
	{0x79, 0x49, 0x49, 0x49, 0x4F},
	{0x7F, 0x49, 0x49, 0x49, 0x4F},
	{0x40, 0x40, 0x40, 0x40, 0x7F},
	{0x7F, 0x49, 0x49, 0x49, 0x7F},
	{0x78, 0x48, 0x48, 0x48, 0x7F},
	{0x7F, 0x48, 0x48, 0x48, 0x48}
};

int dot_init(void);
void dot_write(int);
void dot_clear(void);
void dot_exit(void);

int fnd_init(void);
void fnd_clear(void);
void fnd_exit(void);



int readch(void);
unsigned char hexn2fnd(char ch);
void fnd_display(char *hexadecimal, int N);



int            fd, CG_or_DD = 1;
int hero_pos = 0x00;//global
int gun_pos = 0x00;//global
int p1, p2;//global

unsigned char    *keyin, *keyout;
unsigned char val;
unsigned char keypad_matrix[4][4] = { { '0','1','2','3' },{ '4','5','6','7' },
{ '8','9','A','B' },{ 'C','D','E','F' } };
unsigned char count[3]={0,0,0};
unsigned char keypad_code_matrix[4][4] =
{ { 0x3f,0x06,0x5b,0x4f },{ 0x66,0x6d,0x7d,0x07 },
{ 0x7f,0x67,0x77,0x7c },{ 0x39,0x5e,0x79,0x71 } };


int main(int argc, char **argv) {


		int game_reset;
	unsigned int monster[] = { 0x00, 0x0E, 0x15, 0x1F, 0x0E, 0x15, 0x15, 0x15,0x00,
		0x0E,0x1A,0x1E,0x04,0x1C,0x04,0x0A,0x11,
		0x0E,0x1A,0x1E,0x04,0x1C,0x04,0x0A,0x0A,
		0x07,0x04,0x15,0x0E,0x15,0x0A,0x11,0x11,
		0x1C,0x04,0x15,0x0E,0x15,0x0A,0x11,0x11,
		0x0A,0x1F,0x15,0x1F,0x1F,0x1F,0x0A,0x1A,
		0x0A,0x1F,0x15,0x1F,0x1F,0x1F,0x0A,0x0B };

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("failed memory opening CLCD (using mmap)\n");
		exit(1);
	}
LED=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_LED);
if(fnd_init()<0){
			close(fd);
			printf("mmap error in fnd_init()\n");
			return -1;
	}

	if (clcd_init() < 0) {
		close(fd); printf("mmap error in clcd_init()\n");
		return -1;
	}

	keyin = mmap(NULL, 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, KEY_IN_ADDR);
	keyout = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, KEY_OUT_ADDR);


	if ((int)keyin < 0 || (int)keyout < 0) {
		keyin = NULL;  keyout = NULL;   close(fd);
		printf("mmap error!\n");   return -1;
	}
	
	dot_init();
	fnd_clear();
initialize_clcd();
	CG_clear();
	create(monster);
while(1){
val=0xFF;
  *LED=~val;
usleep(50);
printf("%d %d %d\n",count[2],count[1],count[0]);
count[0]=count[1]=count[2]=0;
fnd_display(count, 3);
printf("%d %d %d\n",count[2],count[1],count[0]);
	game_reset=level_normal(2);
printf("%d %d %d\n",count[2],count[1],count[0]);









unsigned char out_Keypressed;
	int col_no;
  //error////////////////////////////////////////////////////////


while(1){


			switch (*keyin & 0x0f) {
			case 0x02: out_Keypressed = keypad_matrix[0][0]; break;
			case 0x01: out_Keypressed = keypad_matrix[0][1]; break;
			default: out_Keypressed = '?'; break;
			}

			if (out_Keypressed != '?'){if(out_Keypressed=='0')break;
							else if(out_Keypressed=='1') exit(1);}


usleep(10000);
		}


sleep(1);		

	 //error////////////////////////////////////////////////////////
}






	munmap(keyin, 2);  munmap(keyout, 2);  
	clcd_exit();
dot_exit();

		fnd_exit();


	return 0;
}

static void score(void){


				count[0]++;

				if(count[0] > 9){
					count[0]=0; count[1]++;
					if(count[1]>9){
					count[1]=count[0]=0; count[2]++;
					}
				}
fnd_display(count, 3);
}

static void end_print(void){
char buf[10]="GAME  OVER";
int j;
setcommand(0x0000);
usleep(50);
set_RAM_address(0x03,CG_or_DD);
for(j=0;j<10;j++)
{
write_byte(buf[j]);
}
}




static void spawn(int monster1, int position) {
	if (((position<16) && (position>-1)) || ((position<80) && (position>63))) {
		set_RAM_address(position, 1);
		*CLCD_DATA = monster1;
		usleep(50);
	}
}
//

static void level_end(int level){
int standard=0x31+level-1;
int i;
    char buf1[5] = "LEVEL", buf2[6] = " CLEAR", buf3[6]=" START", buf4[6]="FINAL ";
			set_RAM_address(0x02, CG_or_DD);
			  for (i = 0; i < 5; i++){
        write_byte(buf1[i]); 
    }
*CLCD_DATA=standard;
usleep(50);
		  for (i = 0; i < 6; i++){
        write_byte(buf2[i]); 
    }



		set_RAM_address(0x42, CG_or_DD);
if(level<4){
			  for (i = 0; i < 5; i++){
        write_byte(buf1[i]); 
    }

*CLCD_DATA=standard+1;
usleep(50);

		  for (i = 0; i < 6; i++){
        write_byte(buf3[i]); 
    }

}
else
{
			  for (i = 0; i < 6; i++){
        write_byte(buf4[i]); 
    }



		  for (i = 0; i < 6; i++){
        write_byte(buf3[i]); 
    }

}
sleep(1);
}



static int level_normal(int exlevel) {  

	int r,i;
    char buf1[5] = "LEVEL", buf2[6]=" START";
			set_RAM_address(0x02, CG_or_DD);
			  for (i = 0; i < 5; i++){
        write_byte(buf1[i]); 
    }
*CLCD_DATA=0x31;
usleep(50);
		  for (i = 0; i < 6; i++){
        write_byte(buf2[i]); 
    }
sleep(1);

int position_insert;
	  int level=exlevel;
dot_level=1;
	int mon_amount = 6;
	int hero = 0x0000;
	int monster1 = 0x0001;
	int monster2 = 0x0002;
	int ud[22];
	int delete[22]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

	for (i = 0; i < mon_amount; i++) {
		position_insert = 0x0F;
		r = rand() % (level*2);

		if (r>level-1)
		{
			position_insert = position_insert & 0x0F;
		}
		else
		{
			position_insert = position_insert | 0x40;
		}
		ud[i] = position_insert+1;
	}

 hero_pos = 0x00;
 gun_pos = 0x00;
	int level_time = 6 * mon_amount + 25;
	int level_speed = 60000;
	

	unsigned char *keyin0, Key_pressed;
	int col_no;		
	set_RAM_address(hero_pos, CG_or_DD);
		*CLCD_DATA = hero;
		usleep(50);

dot_write(dot_level-1);
usleep(50);

	for (i = 0; i < level_time; i++) {

if(ud[mon_amount-1]==0x00){
printf("end\n");

break;

}
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);

		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));
			*keyin0 = *keyin;
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p1)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p1)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p1)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p1)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p1)) {
						delete[5] = 0;
						break;
					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster1, ud[0]);
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster1, ud[1]);

			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				ud[1] = 0;


			}
		}
		if ((i > ((level_time - 25) / mon_amount)*2-1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster1, ud[2]);
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				ud[2] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster1, ud[3]);
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				ud[3] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster1, ud[4]);
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				ud[4] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster1, ud[5]);
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				ud[5] = 0;

			}
		}
		


		usleep(level_speed);//\C1߰\A3\BAκ\D0



		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));
			*keyin0 = *keyin;
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}

		if (Key_pressed != '?') {
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p2 = 0; p2<15; p2++) {
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0] & 0x0010) != 0x0010) && (ud[0] == gun_pos + 1 + p2)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p2)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p2)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p2)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p2)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p2)) {
						delete[5] = 0;
						break;
					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster2, ud[0]);
				ud[0]--;
				if((ud[0]==0x00)||(ud[0]==0x40)){
				printf("game over\n");
				end_print();
				
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				ud[0] = 0;

			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster2, ud[1]);
				ud[1]--;
				if((ud[1]==0x00)||(ud[1]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				ud[1] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 2 - 1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster2, ud[2]);
				ud[2]--;
				if((ud[2]==0x00)||(ud[2]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				ud[2] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster2, ud[3]);
				ud[3]--;
				if((ud[3]==0x00)||(ud[3]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				ud[3] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster2, ud[4]);
				ud[4]--;
				if((ud[4]==0x00)||(ud[4]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				ud[4] = 0;

			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster2, ud[5]);
				ud[5]--;
				if((ud[5]==0x00)||(ud[5]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				ud[5] = 0;

			}
		}
		

		usleep(level_speed);		
setcommand(0x0001);
usleep(50);
	}
	hero_pos = gun_pos=0x00;

//11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111
	level_end(1);
mon_amount=mon_amount+2;
level++;
	level_time = 6 * mon_amount + 25;
	level_speed = 50000;
	setcommand(0x0000);
for(i=0;i<22;i++){
delete[i]=1;
}
for (i = 0; i < mon_amount; i++) {
		position_insert = 0x0F;
		r = rand() % (level*2);

		if (r>level-1)
		{
			position_insert = position_insert & 0x0F;
		}
		else
		{
			position_insert = position_insert | 0x40;
		}
		ud[i] = position_insert+1;
	}


dot_write(dot_level);
usleep(50);

			
	set_RAM_address(hero_pos, CG_or_DD);
		*CLCD_DATA = hero;
		usleep(50);


	for (i = 0; i < level_time; i++) {

if(ud[mon_amount-1]==0x00){
printf("end\n");

break;

}
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);

		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p1)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p1)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p1)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p1)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p1)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p1)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p1)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p1)) {
						delete[8] = 0;
						break;
					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster1, ud[0]);
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster1, ud[1]);
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount)*2-1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster1, ud[2]);
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster1, ud[3]);
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster1, ud[4]);
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster1, ud[5]);
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster1, ud[6]);
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster1, ud[7]);
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster1, ud[8]);
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				ud[8] = 0;
			}
		}
		


		usleep(level_speed);//\C1߰\A3\BAκ\D0



		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));
			*keyin0 = *keyin;
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}

		if (Key_pressed != '?') {
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {

				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p2 = 0; p2<15; p2++) {
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0] & 0x0010) != 0x0010) && (ud[0] == gun_pos + 1 + p2)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p2)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p2)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p2)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p2)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p2)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p2)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p2)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p2)) {
						delete[8] = 0;
						break;
					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster2, ud[0]);
				ud[0]--;
				if((ud[0]==0x00)||(ud[0]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster2, ud[1]);
				ud[1]--;
				if((ud[1]==0x00)||(ud[1]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 2 - 1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster2, ud[2]);
				ud[2]--;
				if((ud[2]==0x00)||(ud[2]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster2, ud[3]);
				ud[3]--;
				if((ud[3]==0x00)||(ud[3]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster2, ud[4]);
				ud[4]--;
				if((ud[4]==0x00)||(ud[4]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster2, ud[5]);
				ud[5]--;
				if((ud[5]==0x00)||(ud[5]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster2, ud[6]);
				ud[6]--;
				if((ud[6]==0x00)||(ud[6]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster2, ud[7]);
				ud[7]--;
				if((ud[7]==0x00)||(ud[7]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster2, ud[8]);
				ud[8]--;
				if((ud[8]==0x00)||(ud[8]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				ud[8] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 9 - 1) && (ud[9] > 0))
		{
			if (delete[9] != 0) {
				spawn(monster2, ud[9]);
				ud[9]--;
				if((ud[9]==0x00)||(ud[9]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[9]);
				score();
				ud[9] = 0;
			}
		}
		

		usleep(level_speed);		
setcommand(0x0001);
usleep(50);
	}
	hero_pos = gun_pos=0x00;
//222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222

level_end(2);
	 monster1 = 0x0003;
	 monster2 = 0x0004;
mon_amount=mon_amount+4;
level++;
	level_time = 4 * mon_amount + 25;
	level_speed = 40000;
	setcommand(0x0000);
for(i=0;i<22;i++){
delete[i]=1;
}
for (i = 0; i < mon_amount; i++) {
		position_insert = 0x0F;
		r = rand() % (level*2);

		if (r>level-1)
		{
			position_insert = position_insert & 0x0F;
		}
		else
		{
			position_insert = position_insert | 0x40;
		}
		ud[i] = position_insert+1;
	}

dot_level++;
dot_write(dot_level);
usleep(50);

			
	set_RAM_address(hero_pos, CG_or_DD);
		*CLCD_DATA = hero;
		usleep(50);
	for (i = 0; i < level_time; i++) {

if(ud[mon_amount-1]==0x00){
printf("end\n");

break;

}
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);

		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p1)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p1)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p1)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p1)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p1)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p1)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p1)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p1)) {
						delete[8] = 0;
						break;
					}
					if (((ud[9] & 0x0010) != 0x0010) && (ud[9] == gun_pos + 1 + p1)) {
						delete[9] = 0;
						break;
					}
					if (((ud[10] & 0x0010) != 0x0010) && (ud[10] == gun_pos + 1 + p1)) {
						delete[10] = 0;
						break;
					}
	if (((ud[11] & 0x0010) != 0x0010) && (ud[11] == gun_pos + 1 + p1)) {
						delete[11] = 0;
						break;
					}
					if (((ud[12] & 0x0010) != 0x0010) && (ud[12] == gun_pos + 1 + p1)) {
						delete[12] = 0;
						break;
					}
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster1, ud[0]);
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster1, ud[1]);
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount)*2-1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster1, ud[2]);
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster1, ud[3]);
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster1, ud[4]);
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster1, ud[5]);
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster1, ud[6]);
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster1, ud[7]);
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster1, ud[8]);
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				score();
				ud[8] = 0;
			}
		}	if ((i > ((level_time - 25) / mon_amount ) * 9 - 1) && (ud[9] > 0))
		{
			if (delete[9] != 0) {
				spawn(monster1, ud[9]);
			}
			else
			{
				spawn(0x002A, ud[9]);
				score();
				score();
				ud[9] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 10 - 1) && (ud[10] > 0))
		{
			if (delete[10] != 0) {
				spawn(monster1, ud[10]);
			}
			else
			{
				spawn(0x002A, ud[10]);
				score();
				score();
				ud[10] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 11 - 1) && (ud[11] > 0))
		{
			if (delete[11] != 0) {
				spawn(monster1, ud[11]);
			}
			else
			{
				spawn(0x002A, ud[11]);
				score();
				score();
				ud[11] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 12 - 1) && (ud[12] > 0))
		{
			if (delete[12] != 0) {
				spawn(monster1, ud[12]);
			}
			else
			{
				spawn(0x002A, ud[12]);
				score();
				score();
				ud[12] = 0;
			}
		}


		usleep(level_speed);//\C1߰\A3\BAκ\D0



		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));
			*keyin0 = *keyin;
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}

		if (Key_pressed != '?') {
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p2 = 0; p2<15; p2++) {
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0] & 0x0010) != 0x0010) && (ud[0] == gun_pos + 1 + p2)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p2)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p2)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p2)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p2)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p2)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p2)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p2)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p2)) {
						delete[8] = 0;
						break;
					}
					if (((ud[9] & 0x0010) != 0x0010) && (ud[9] == gun_pos + 1 + p2)) {
						delete[9] = 0;
						break;
					}
					if (((ud[10] & 0x0010) != 0x0010) && (ud[10] == gun_pos + 1 + p2)) {
						delete[10] = 0;
						break;
					}
					if (((ud[11] & 0x0010) != 0x0010) && (ud[11] == gun_pos + 1 + p2)) {
						delete[11] = 0;
						break;
					}
					if (((ud[12] & 0x0010) != 0x0010) && (ud[12] == gun_pos + 1 + p2)) {
						delete[12] = 0;
						break;
					}
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster2, ud[0]);
				ud[0]--;
				if((ud[0]==0x00)||(ud[0]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster2, ud[1]);
				ud[1]--;
				if((ud[1]==0x00)||(ud[1]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 2 - 1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster2, ud[2]);
				ud[2]--;
				if((ud[2]==0x00)||(ud[2]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster2, ud[3]);
				ud[3]--;
				if((ud[3]==0x00)||(ud[3]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster2, ud[4]);
				ud[4]--;
				if((ud[4]==0x00)||(ud[4]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster2, ud[5]);
				ud[5]--;
				if((ud[5]==0x00)||(ud[5]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster2, ud[6]);
				ud[6]--;
				if((ud[6]==0x00)||(ud[6]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster2, ud[7]);
				ud[7]--;
				if((ud[7]==0x00)||(ud[7]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster2, ud[8]);
				ud[8]--;
				if((ud[8]==0x00)||(ud[8]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				score();
				ud[8] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 9 - 1) && (ud[9] > 0))
		{
			if (delete[9] != 0) {
				spawn(monster2, ud[9]);
				ud[9]--;
				if((ud[9]==0x00)||(ud[9]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[9]);
				score();
				score();
				ud[9] = 0;
			}
		}
			if ((i > ((level_time - 25) / mon_amount ) * 10 - 1) && (ud[10] > 0))
		{
			if (delete[10] != 0) {
				spawn(monster2, ud[10]);
				ud[10]--;
				if((ud[10]==0x00)||(ud[10]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[10]);
				score();
				score();
				ud[10] = 0;
			}
		}
if ((i > ((level_time - 25) / mon_amount ) * 11 - 1) && (ud[11] > 0))
		{
			if (delete[11] != 0) {
				spawn(monster2, ud[11]);
				ud[11]--;
				if((ud[11]==0x00)||(ud[11]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[11]);
				score();
				score();
				ud[11] = 0;
			}
		}
			if ((i > ((level_time - 25) / mon_amount ) * 12 - 1) && (ud[12] > 0))
		{
			if (delete[12] != 0) {
				spawn(monster2, ud[12]);
				ud[12]--;
				if((ud[12]==0x00)||(ud[12]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[12]);
				score();
				score();
				ud[12] = 0;
			}
		}
		usleep(level_speed);		
setcommand(0x0001);
usleep(50);
	}
	hero_pos = gun_pos=0x00;
//333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333
level_end(3);

mon_amount=mon_amount+2;
level++;
	level_time = 4 * mon_amount + 25;
	level_speed = 30000;
	setcommand(0x0000);
for(i=0;i<22;i++){
delete[i]=1;
}
for (i = 0; i < mon_amount; i++) {
		position_insert = 0x0F;
		r = rand() % (level*2);

		if (r>level-1)
		{
			position_insert = position_insert & 0x0F;
		}
		else
		{
			position_insert = position_insert | 0x40;
		}
		ud[i] = position_insert+1;
	}


dot_level++;
dot_write(dot_level);
usleep(50);

			
	set_RAM_address(hero_pos, CG_or_DD);
		*CLCD_DATA = hero;
		usleep(50);
	for (i = 0; i < level_time; i++) {

if(ud[mon_amount-1]==0x00){
printf("end\n");

break;

}
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);

		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p1)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p1)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p1)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p1)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p1)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p1)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p1)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p1)) {
						delete[8] = 0;
						break;
					}
					if (((ud[9] & 0x0010) != 0x0010) && (ud[9] == gun_pos + 1 + p1)) {
						delete[9] = 0;
						break;
					}
					if (((ud[10] & 0x0010) != 0x0010) && (ud[10] == gun_pos + 1 + p1)) {
						delete[10] = 0;
						break;
					}
	if (((ud[11] & 0x0010) != 0x0010) && (ud[11] == gun_pos + 1 + p1)) {
						delete[11] = 0;
						break;
					}
					if (((ud[12] & 0x0010) != 0x0010) && (ud[12] == gun_pos + 1 + p1)) {
						delete[12] = 0;
						break;
					}
					if (((ud[13] & 0x0010) != 0x0010) && (ud[13] == gun_pos + 1 + p1)) {
						delete[13] = 0;
						break;
					}
					if (((ud[14] & 0x0010) != 0x0010) && (ud[14] == gun_pos + 1 + p1)) {
						delete[14] = 0;
						break;
					}
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster1, ud[0]);
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster1, ud[1]);
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount)*2-1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster1, ud[2]);
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster1, ud[3]);
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster1, ud[4]);
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster1, ud[5]);
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster1, ud[6]);
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster1, ud[7]);
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster1, ud[8]);
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				score();
				ud[8] = 0;
			}
		}	if ((i > ((level_time - 25) / mon_amount ) * 9 - 1) && (ud[9] > 0))
		{
			if (delete[9] != 0) {
				spawn(monster1, ud[9]);
			}
			else
			{
				spawn(0x002A, ud[9]);
				score();
				score();
				ud[9] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 10 - 1) && (ud[10] > 0))
		{
			if (delete[10] != 0) {
				spawn(monster1, ud[10]);
			}
			else
			{
				spawn(0x002A, ud[10]);
				score();
				score();
				ud[10] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 11 - 1) && (ud[11] > 0))
		{
			if (delete[11] != 0) {
				spawn(monster1, ud[11]);
			}
			else
			{
				spawn(0x002A, ud[11]);
				score();
				score();
				ud[11] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 12 - 1) && (ud[12] > 0))
		{
			if (delete[12] != 0) {
				spawn(monster1, ud[12]);
			}
			else
			{
				spawn(0x002A, ud[12]);
				score();
				score();
				ud[12] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 13 - 1) && (ud[13] > 0))
		{
			if (delete[13] != 0) {
				spawn(monster1, ud[13]);
			}
			else
			{
				spawn(0x002A, ud[13]);
				score();
				score();
				ud[13] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 14 - 1) && (ud[14] > 0))
		{
			if (delete[14] != 0) {
				spawn(monster1, ud[14]);
			}
			else
			{
				spawn(0x002A, ud[14]);
				score();
				score();
				ud[14] = 0;
			}
		}

		usleep(level_speed);//\C1߰\A3\BAκ\D0



		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));
			*keyin0 = *keyin;
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}

		if (Key_pressed != '?') {
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p2 = 0; p2<15; p2++) {
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p2, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0] & 0x0010) != 0x0010) && (ud[0] == gun_pos + 1 + p2)) {
						delete[0] = 0;
						break;
					}
					if (((ud[1] & 0x0010) != 0x0010) && (ud[1] == gun_pos + 1 + p2)) {
						delete[1] = 0;
						break;
					}
					if (((ud[2] & 0x0010) != 0x0010) && (ud[2] == gun_pos + 1 + p2)) {
						delete[2] = 0;
						break;
					}
					if (((ud[3] & 0x0010) != 0x0010) && (ud[3] == gun_pos + 1 + p2)) {
						delete[3] = 0;
						break;
					}
					if (((ud[4] & 0x0010) != 0x0010) && (ud[4] == gun_pos + 1 + p2)) {
						delete[4] = 0;
						break;
					}
					if (((ud[5] & 0x0010) != 0x0010) && (ud[5] == gun_pos + 1 + p2)) {
						delete[5] = 0;
						break;
					}
					if (((ud[6] & 0x0010) != 0x0010) && (ud[6] == gun_pos + 1 + p2)) {
						delete[6] = 0;
						break;
					}
					if (((ud[7] & 0x0010) != 0x0010) && (ud[7] == gun_pos + 1 + p2)) {
						delete[7] = 0;
						break;
					}
					if (((ud[8] & 0x0010) != 0x0010) && (ud[8] == gun_pos + 1 + p2)) {
						delete[8] = 0;
						break;
					}
					if (((ud[9] & 0x0010) != 0x0010) && (ud[9] == gun_pos + 1 + p2)) {
						delete[9] = 0;
						break;
					}
					if (((ud[10] & 0x0010) != 0x0010) && (ud[10] == gun_pos + 1 + p2)) {
						delete[10] = 0;
						break;
					}
					if (((ud[11] & 0x0010) != 0x0010) && (ud[11] == gun_pos + 1 + p2)) {
						delete[11] = 0;
						break;
					}
					if (((ud[12] & 0x0010) != 0x0010) && (ud[12] == gun_pos + 1 + p2)) {
						delete[12] = 0;
						break;
					}if (((ud[13] & 0x0010) != 0x0010) && (ud[13] == gun_pos + 1 + p2)) {
						delete[13] = 0;
						break;
					}
					if (((ud[14] & 0x0010) != 0x0010) && (ud[14] == gun_pos + 1 + p2)) {
						delete[14] = 0;
						break;
					}
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster2, ud[0]);
				ud[0]--;
				if((ud[0]==0x00)||(ud[0]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[0]);
				score();
				score();
				ud[0] = 0;
			}
		}
		if ((i > (level_time - 25) / mon_amount-1) && (ud[1] > 0))
		{
			if (delete[1] != 0) {
				spawn(monster2, ud[1]);
				ud[1]--;
				if((ud[1]==0x00)||(ud[1]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[1]);
				score();
				score();
				ud[1] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 2 - 1) && (ud[2] > 0))
		{
			if (delete[2] != 0) {
				spawn(monster2, ud[2]);
				ud[2]--;
				if((ud[2]==0x00)||(ud[2]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[2]);
				score();
				score();
				ud[2] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 3 - 1) && (ud[3] > 0))
		{
			if (delete[3] != 0) {
				spawn(monster2, ud[3]);
				ud[3]--;
				if((ud[3]==0x00)||(ud[3]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[3]);
				score();
				score();
				ud[3] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 4 - 1) && (ud[4] > 0))
		{
			if (delete[4] != 0) {
				spawn(monster2, ud[4]);
				ud[4]--;
				if((ud[4]==0x00)||(ud[4]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[4]);
				score();
				score();
				ud[4] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 5 - 1) && (ud[5] > 0))
		{
			if (delete[5] != 0) {
				spawn(monster2, ud[5]);
				ud[5]--;
				if((ud[5]==0x00)||(ud[5]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[5]);
				score();
				score();
				ud[5] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 6 - 1) && (ud[6] > 0))
		{
			if (delete[6] != 0) {
				spawn(monster2, ud[6]);
				ud[6]--;
				if((ud[6]==0x00)||(ud[6]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[6]);
				score();
				score();
				ud[6] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 7 - 1) && (ud[7] > 0))
		{
			if (delete[7] != 0) {
				spawn(monster2, ud[7]);
				ud[7]--;
				if((ud[7]==0x00)||(ud[7]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[7]);
				score();
				score();
				ud[7] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 8 - 1) && (ud[8] > 0))
		{
			if (delete[8] != 0) {
				spawn(monster2, ud[8]);
				ud[8]--;
				if((ud[8]==0x00)||(ud[8]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[8]);
				score();
				score();
				ud[8] = 0;
			}
		}
		if ((i > ((level_time - 25) / mon_amount ) * 9 - 1) && (ud[9] > 0))
		{
			if (delete[9] != 0) {
				spawn(monster2, ud[9]);
				ud[9]--;
				if((ud[9]==0x00)||(ud[9]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[9]);
				score();
				score();
				ud[9] = 0;
			}
		}
			if ((i > ((level_time - 25) / mon_amount ) * 10 - 1) && (ud[10] > 0))
		{
			if (delete[10] != 0) {
				spawn(monster2, ud[10]);
				ud[10]--;
				if((ud[10]==0x00)||(ud[10]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[10]);
				score();
				score();
				ud[10] = 0;
			}
		}
if ((i > ((level_time - 25) / mon_amount ) * 11 - 1) && (ud[11] > 0))
		{
			if (delete[11] != 0) {
				spawn(monster2, ud[11]);
				ud[11]--;
				if((ud[11]==0x00)||(ud[11]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[11]);
				score();
				score();
				ud[11] = 0;
			}
		}
			if ((i > ((level_time - 25) / mon_amount ) * 12 - 1) && (ud[12] > 0))
		{
			if (delete[12] != 0) {
				spawn(monster2, ud[12]);
				ud[12]--;
				if((ud[12]==0x00)||(ud[12]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[12]);
				score();
				score();
				ud[12] = 0;
			}
		}
			
			if ((i > ((level_time - 25) / mon_amount ) * 13 - 1) && (ud[13] > 0))
		{
			if (delete[13] != 0) {
				spawn(monster2, ud[13]);
				ud[13]--;
				if((ud[13]==0x00)||(ud[13]==0x40)){
				printf("game over\n");
				end_print();
				return 0;
				}
			}
			else
			{
				spawn(0x002A, ud[13]);
				score();
				score();
				ud[13] = 0;
			}
		}
		usleep(level_speed);		
setcommand(0x0001);
usleep(50);
	}
	hero_pos = gun_pos=0x00;
level_end(4);
//4444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444444
monster1=0x0005;
monster2=0x0006;
	setcommand(0x0000);
for(i=0;i<22;i++){
delete[0]=8;
}
for (i = 0; i < mon_amount; i++) {
		position_insert = 0x0F;
		r = rand() % (level*2);

		if (r>level-1)
		{
			position_insert = position_insert & 0x0F;
		}
		else
		{
			position_insert = position_insert | 0x40;
		}
		ud[i] = position_insert+1;
	}


dot_write(dot_level+6);
usleep(50);
	set_RAM_address(hero_pos, CG_or_DD);
		*CLCD_DATA = hero;
		usleep(50);
	for (i = 0; i < 16; i++) {


				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);

		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
	if(delete[0]==0){
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}
						delete[0]--;

val=val<<1;
  						*LED=~val; 

break;
					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster1, ud[0]);
			}
else{
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}
			
		}
		
	

		usleep(50000);//\C1߰\A3\BAκ\D0

	for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
		if(delete[0]==0){
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}
						delete[0]--;
val=val<<1;
  						*LED=~val; 
break;


					}
					
				}


			}


		}
usleep(50000);


		for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
		if(delete[0]==0){
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}

						delete[0]--;
val=val<<1;
  						*LED=~val; 
break;


					}
					
				}


			}


		}


		if (ud[0]>0) {
			if (delete[0] != 0)
			{
				spawn(monster2, ud[0]);
				ud[0]--;
				if((ud[0]==0x00)||(ud[0]==0x40)){
				printf("game over\n");
				end_print();

return 0;
				}
			}
else{
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}
			



		}

		usleep(50000);//\C1߰\A3\BAκ\D0


	for (col_no = 0; col_no<SCAN_NUM; col_no++) {
			*keyout = (1 << (SCAN_NUM - 1 - col_no));

			*keyin0 = *keyin;
			
			switch (*keyin0 & 0x0f) {
			case 0x01: Key_pressed = keypad_matrix[0][col_no]; break;
			case 0x02: Key_pressed = keypad_matrix[1][col_no]; break;
			case 0x04: Key_pressed = keypad_matrix[2][col_no]; break;
			case 0x08: Key_pressed = keypad_matrix[3][col_no]; break;
			default: Key_pressed = '?'; break;
			}
			if (Key_pressed != '?')break;
		}


		if (Key_pressed != '?') { 
			if (Key_pressed == 'C') {
				set_RAM_address(0x00, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x40;
				gun_pos = 0x40;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '8') {
				set_RAM_address(0x40, CG_or_DD);
				*CLCD_DATA = 0x0007;
				usleep(50);
				hero_pos = 0x00;
				gun_pos = 0x00;
				set_RAM_address(hero_pos, CG_or_DD);
				*CLCD_DATA = hero;
				usleep(50);
			}
			else if (Key_pressed == '4') {
				for (p1 = 0; p1<15; p1++) { 
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x003E;
					usleep(10000);
					set_RAM_address(gun_pos + 1 + p1, CG_or_DD);
					*CLCD_DATA = 0x0007;
					usleep(50);
					if (((ud[0]&0x0010)!=0x0010)&&(ud[0] == gun_pos + 1 + p1)) {
if(delete[0]==0){
				spawn(0x002A, ud[0]);
			printf("game clear\n");
count[2]++;fnd_display(count,3);

clear_print();
exit(1);

			}
						delete[0]--;
val=val<<1;
  						*LED=~val; 
break;


					}
					
				}


			}


		}

			usleep(50000);
r = rand() % (level*2);

		if (r>level-1)
		{
			ud[0] = ud[0] & 0x0F;
		}
		else
		{
			ud[0] = ud[0] | 0x40;
		}

setcommand(0x0001);
usleep(50);

	}
	hero_pos = gun_pos=0x00;
usleep(100000);
return 0;
}

static void clear_print(void){

int i;
    char buf2[6] = " CLEAR", buf1[6]="FINAL ";
setcommand(0x0000);
usleep(50);
			set_RAM_address(0x02, CG_or_DD);
			  for (i = 0; i < 6; i++){
        write_byte(buf1[i]); 
    }
usleep(50);
		  for (i = 0; i < 6; i++){
        write_byte(buf2[i]); 
    }

	
sleep(1);
}


int clcd_init(void) {
	int ierr = 0;
	CLCD_CMD = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_WR);
	CLCD_DATA = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_CLCD_RS);
	ierr = (int)((!CLCD_CMD) || (CLCD_DATA));
	return ierr;
}

void write_byte(char ch) {
	unsigned short data;
	data = ch & 0x00FF;
	*CLCD_DATA = data;
	usleep(50);
}
static void create(unsigned int* character) {
	unsigned short data;
	int i;
	*CLCD_CMD = 0x40;
	for (i = 0; i <57; i++) {
		data = character[i] | 0x0200;
		*CLCD_DATA = data;
		usleep(50);
	}
}

static void CG_clear(void) {
	unsigned short data;
	int i;
	*CLCD_CMD = 0x40;
	for (i = 0; i <80; i++) {
		data = 0x0200;
		*CLCD_DATA = data;
		usleep(50);
	}
}

static void setcommand(unsigned short command) {
	command &= 0x00FF; *CLCD_CMD = command; usleep(2000);
}

static void initialize_clcd(void) {
	int DL = 1, N = 1, F = 0, D = 1, C = 0, B = 0, ID = 1, S = 0;
	function_set(DL, N, F);
	display_control(D, C, B);
	clcd_clear();
	entry_mode_set(ID, S);
	return_home();
}

static void function_set(int DL, int N, int F) {
	unsigned short command = 0x20;
	if (DL  > 0) command |= 0x10;
	if (N   > 0) command |= 0x08;
	if (F   > 0) command |= 0x04;
	setcommand(command);
}

static void display_control(int D, int C, int B) {
	unsigned short command = 0x08;
	if (D   > 0) command |= 0x04;
	if (C   > 0) command |= 0x02;
	if (B   > 0) command |= 0x01;
	setcommand(command);
}

static void cursor_shift(int set_screen, int set_rightshift) {
	unsigned short command = 0x10;
	if (set_screen > 0)     command |= 0x08;
	if (set_rightshift > 0) command |= 0x04;
	setcommand(command);
}

static void entry_mode_set(int ID, int S) {
	unsigned short command = 0x04;
	if (ID > 0) command |= 0x02;
	if (S  > 0) command |= 0x01;
	setcommand(command);
}

static void return_home(void) {
	setcommand(0x02);
}

static void clcd_clear(void) {
	setcommand(0x01);
}

static set_RAM_address(int pos, int CG_or_DD) {
	unsigned short command = 0x00;
	if (CG_or_DD > 0) command = 0x80;
	command |= pos;
	setcommand(command);
}

void clcd_exit(void) {
	munmap(CLCD_CMD, 2);
	munmap(CLCD_DATA, 2);
	close(fd);
}
int dot_init(void){
	int ierr=0;
	DOT_COL1 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL1);
	DOT_COL2 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL2);
	DOT_COL3 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL3);
	DOT_COL4 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL4);
	DOT_COL5 = mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FPGA_DOT_COL5);
	ierr=(int)DOT_COL1+(int)DOT_COL2+(int)DOT_COL3+(int)DOT_COL4+(int)DOT_COL5;
	return ierr;
}

int fnd_init(void){
	int ierr=0;
	FND0=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS0); ierr+=(int)FND0;
	FND1=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS1); ierr+=(int)FND1;
	FND2=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS2); ierr+=(int)FND2;
	FND3=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS3); ierr+=(int)FND3;
	FND4=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS4); ierr+=(int)FND4;
	FND5=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS5); ierr+=(int)FND5;
	FND6=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS6); ierr+=(int)FND6;
	FND7=mmap(NULL, 2, PROT_WRITE, MAP_SHARED, fd, FND_CS7); ierr+=(int)FND7;
	return ierr;
}






void dot_write(int decimal){
	*DOT_COL1 = dot_table[decimal][0];
	*DOT_COL2 = dot_table[decimal][1];
	*DOT_COL3 = dot_table[decimal][2];
	*DOT_COL4 = dot_table[decimal][3];
	*DOT_COL5 = dot_table[decimal][4];
}

unsigned char hexn2fnd(char ch){
	unsigned char code;
	switch(ch){
		case 0x00: code=0x3f; break;
		case 0x01: code=0x06; break;
		case 0x02: code=0x5b; break;
		case 0x03: code=0x4f; break;
		case 0x04: code=0x66; break;
		case 0x05: code=0x6d; break;
		case 0x06: code=0x7d; break;
		case 0x07: code=0x07; break;
		case 0x08: code=0x7f; break;
		case 0x09: code=0x67; break;
		case 0x0A: code=0x77; break;
		case 0x0B: code=0x7c; break;
		case 0x0C: code=0x39; break;
		case 0x0D: code=0x5e; break;
		case 0x0E: code=0x79; break;
		case 0x0F: code=0x71; break;
		default : code=0x00;
	}
	return code;
}

void fnd_display(char *hexadecimal, int N){
	if(N>=1) *FND0=hexn2fnd(hexadecimal[0]);
	if(N>=2) *FND1=hexn2fnd(hexadecimal[1]);
	if(N>=3) *FND2=hexn2fnd(hexadecimal[2]);
	if(N>=4) *FND3=hexn2fnd(hexadecimal[3]);
	if(N>=5) *FND4=hexn2fnd(hexadecimal[4]);
	if(N>=6) *FND5=hexn2fnd(hexadecimal[5]);
	if(N>=7) *FND6=hexn2fnd(hexadecimal[6]);
	if(N>=8) *FND7=hexn2fnd(hexadecimal[7]);
}


void dot_clear(void){
	*DOT_COL1 = 0x00;
	*DOT_COL2 = 0x00;
	*DOT_COL3 = 0x00;
	*DOT_COL4 = 0x00;
	*DOT_COL5 = 0x00;
}

void fnd_clear(void){
	*FND0=0x00; *FND1=0x00; *FND2=0x00; *FND3=0x00;
	*FND4=0x00; *FND5=0x00; *FND6=0x00; *FND7=0x00;
}

void dot_exit(void){
	dot_clear();
	munmap(DOT_COL1, 2);
	munmap(DOT_COL2, 2);
	munmap(DOT_COL3, 2);
	munmap(DOT_COL4, 2);
	munmap(DOT_COL5, 2);
	close(fd);
}

void fnd_exit(void){
	fnd_clear();
	munmap(FND0,2); munmap(FND1,2); munmap(FND2,2); munmap(FND3,2);
	munmap(FND4,2); munmap(FND5,2); munmap(FND6,2); munmap(FND7,2);
	close(fd);

}


