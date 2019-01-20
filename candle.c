/*
 *  Candle burner for Uzebox 
 *  Version 1.0
 *  Copyright (C) 2013  Hartmut Wendt
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include "kernel/uzebox.h"

#include "data/candle_Introtiles.pic.inc"
#include "data/fonts.pic.inc"
#include "data/candle_BGtiles.pic.inc"
#include "data/candle_spritetiles.pic.inc"
//#include "data/jingleb.inc"
#include "data/odufrohl.inc"
#include "data/wewishy2.inc"
#include "data/jingle_loop.inc"
#include "data/cburner_patches.inc"


  

//#define _FadingX
#define FONT_OFFSET	7


/* global definitons */
// program modes
enum {
	PM_Intro,			// program mode intro
	PM_Match_wo_Flame,		
	PM_Match_small_Flame,		
	PM_Match_Flame,   	// program mode game play
	PM_Smoking_Match,	// program mode in game smoking match
	PM_Game_finish,
	PM_HoF_edit	
};


// sprite animations
#define ani_match_without_flame_max 2
const char *ani_match_without_flame[] PROGMEM = {
	match_without_flame,
	match_without_flame
};


#define ani_match_burning_big_max 11
const char *ani_match_burning_big[] PROGMEM = {
	match_flame_big_1,
	match_flame_big_1,
	match_flame_big_2,
	match_flame_big_2,
	match_flame_big_1,
	match_flame_big_3,
	match_flame_big_3,
	match_flame_big_1,
	match_flame_big_3,
	match_flame_big_1,	
	match_flame_big_2
};


#define ani_match_burning_small_max 2
const char *ani_match_burning_small[] PROGMEM = {
	match_flame_small_1,
	match_flame_small_2
};


#define ani_match_smoking_max 6
const char *ani_match_smoking[] PROGMEM = {
	match_with_smoke_1,
	match_with_smoke_2,
	match_with_smoke_3,
	match_with_smoke_1,
	match_with_smoke_2,
	match_with_smoke_3
};


#define ani_blinking_star_max 8
const char *ani_blinking_star[] PROGMEM = {
	blinking_star_1,
	blinking_star_2,
	blinking_star_3,
	blinking_star_4,
	blinking_star_3,
	blinking_star_2,
	blinking_star_1,
	empty_sprite_small
};


#define star_coordinates_max 6
const char star_coordinates[star_coordinates_max][2]  = {
    {114, 92},
    {59, 176},
    {62, 65},
    {130, 146},    
    {54, 111},
    {90, 142}   

};




// 8-bit, 255 period LFSR (for generating pseudo-random numbers)
#define PRNG_NEXT() (prng = ((u8)((prng>>1) | ((prng^(prng>>2)^(prng>>3)^(prng>>4))<<7))))
#define MAX(x,y) ((x)>(y) ? (x) : (y))




struct EepromBlockStruct ebs;

struct flame {
	bool flame_enable;
	u8 flame_counter;
	u8 PosX;
	u8 PosY;
	};

/*
struct flame default_flame0 = {true,1,24,19};
struct flame default_flame1 = {true,0,8,6};
struct flame default_flame2 = {true,0,13,6};
struct flame default_flame3 = {true,0,5,12};
struct flame default_flame4 = {true,0,16,13};
struct flame default_flame5 = {true,0,3,19};
struct flame default_flame6 = {false,0,19,19};

*/
struct flame default_flame0 = {true,1,24,19};
struct flame default_flame1 = {false,0,8,6};
struct flame default_flame2 = {false,0,13,6};
struct flame default_flame3 = {false,0,5,12};
struct flame default_flame4 = {false,0,16,13};
struct flame default_flame5 = {false,0,3,19};
struct flame default_flame6 = {false,0,19,19};


	

u8 prng; // Pseudo-random number generator

u8 program_mode;	// program mode (intro, 1 player mode, 2 player mode, ....



u8 PosY=120;
u8 PosX=112;
u8 sprite_ani_cnt = 0;
u8 sprite_ani_max = 0;
char *sprite_animation[12];
u8 X_pos_sprite=0;
u8 Y_pos_sprite=0;
u8 Match_speed = 1;
struct flame candles[7];
int stop_watch_tmr;

 

/*** function prototypes *****************************************************************/
void init(void);
void set_PM_mode(u8 mode);
void msg_window(u8 x1, u8 y1, u8 x2, u8 y2);

u8 GetTile(u8 x, u8 y);
void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ);
void fill_buf(u8 *BUFA, u8 content, u8 ucANZ);
void button_handler(u8 PosX, u8 PosY);
void animate_sprite(void);
void animate_candles(void);
void CandlePrintByte(int x,int y, unsigned char val,bool zeropad);
void CandlePrintInt(int x,int y, unsigned int val,bool zeropad, u8 digits);
void view_stop_watch(int x,int y,int time_value);
void view_stop_watch_title(int x,int y,int time_value);
u8 set_def_highscore(void);
u8 check_highscore(void);
void copy_highsore(u8 entry_from, u8 entry_to);
void clear_highsore(u8 entry);
u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data);
void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode);
void show_highscore_char(u8 entry, u8 position, u8 cursor_on);
void burning_tree(void);

void init(void)
// init program
{

  // init music	
  InitMusicPlayer(patches);

     		
   // init random generator
  prng = 1;

  //Use block 27
  ebs.id = 27;
  if (!isEepromFormatted())
     return;

  if (EEPROM_ERROR_BLOCK_NOT_FOUND == EepromReadBlock(27,&ebs))
  {
	set_def_highscore();

  }	

  // load into screen
  set_PM_mode(PM_Intro);

}



int main(){
int ibuttons=0,ibuttons_old;
u8 uc1, uc2=0;
u8 ucCNT, ucIDLE_TMR;

  // init program
  init();        
  
  // proceed program	
  while(1)
  {
    WaitVsync(1);	  
    // get controller state
    ibuttons_old = ibuttons;
	ibuttons = ReadJoypad(0);


    switch(program_mode)
	{
	  // proceed intro mode	
	  case PM_Intro:	    
		// display blinking text:
		if (uc1 >= 14) {
		  Print(9,13,PSTR("PRESS START"));		  
		  uc1 = 0;
	
		} else if (uc1==7) {
		  Print(9,13,PSTR("           "));

		}
		uc1++;
		// check start button
		if (BTN_START & ibuttons) {
          ucIDLE_TMR = 0;
		  set_PM_mode(PM_Match_wo_Flame); 	
		}



 		WaitVsync(1);
		#ifdef _FadingX
	 	FadeOut(1, true);
		#endif
		//set_PM_mode(PM_Win8);
  		#ifdef _FadingX
    	FadeIn(1, true);
		#endif	
        break;

	  // proceed game
	  case PM_Match_wo_Flame:
	  case PM_Match_small_Flame:
	  case PM_Match_Flame:
	  case PM_Smoking_Match:
		WaitVsync(5);

		// handle sprite
		if (Match_speed < 30) Match_speed++;
    	if ((BTN_LEFT & ibuttons) && (X_pos_sprite>10)) X_pos_sprite -= Match_speed >> 2;
    	else if ((BTN_RIGHT & ibuttons) && (X_pos_sprite<220)) X_pos_sprite += Match_speed >> 2;	
	   	else if ((BTN_UP & ibuttons) && (Y_pos_sprite>0)) Y_pos_sprite -= Match_speed >> 2;
    	else if ((BTN_DOWN & ibuttons) && (Y_pos_sprite<191)) Y_pos_sprite += Match_speed >> 2;		
		else Match_speed = 2;


		if ((BTN_LEFT & ibuttons) || (BTN_RIGHT & ibuttons) || (BTN_UP & ibuttons) || (BTN_DOWN & ibuttons)) {
			ucIDLE_TMR = 0;
		} else {
			// no button is pressed
			if (ucIDLE_TMR < 15) ucIDLE_TMR++;


		}



		// choose an animation depending from the speed of the match && program mode
		if (program_mode == PM_Smoking_Match) {
			ucCNT++;
			if (ucCNT > 10) {
				// finish smoking
				program_mode = PM_Match_wo_Flame;						
				sprite_ani_max = ani_match_without_flame_max; 
				sprite_ani_cnt = 0;
				memcpy_P( &sprite_animation[0], &ani_match_without_flame[0], sizeof(sprite_animation) );	

			}

		} else if ((Match_speed == 20) && (program_mode == PM_Match_Flame)) {
			sprite_ani_max = ani_match_burning_small_max; 
			sprite_ani_cnt = 0;
			memcpy_P( &sprite_animation[0], &ani_match_burning_small[0], sizeof(sprite_animation) );
			program_mode = PM_Match_small_Flame;	

		} else if ((Match_speed == 30) && (program_mode == PM_Match_small_Flame)) {
			sprite_ani_max = ani_match_smoking_max; 
			sprite_ani_cnt = 0;
			memcpy_P( &sprite_animation[0], &ani_match_smoking[0], sizeof(sprite_animation) );	
			program_mode = PM_Smoking_Match;
			ucCNT = 0;

		} else if ((Match_speed == 2)  && (program_mode == PM_Match_small_Flame)) {
			sprite_ani_max = ani_match_burning_big_max; 
			sprite_ani_cnt = 0;
			memcpy_P( &sprite_animation[0], &ani_match_burning_big[0], sizeof(sprite_animation) );	
			program_mode = PM_Match_Flame;
		}


        //PrintByte(5,26,Match_speed,true) ;
		animate_sprite();
		animate_candles();

		//CandlePrintByte(5,25,X_pos_sprite,true);
		//CandlePrintByte(10,25,Y_pos_sprite,true);
		//increment the stop watch#
		stop_watch_tmr++;
		view_stop_watch(20,1,stop_watch_tmr);
		
		if (program_mode == PM_Match_Flame) {
			// collision check for tree candles
			for(uc1 = 1; uc1 < 7; uc1++) {
				if (candles[uc1].flame_enable) continue;
				// check x and y position of match
				if (((X_pos_sprite + 11) > (candles[uc1].PosX * 8)) &&	  
			   		((X_pos_sprite + 7)	< (candles[uc1].PosX * 8)) &&	
			   		((Y_pos_sprite + 1)	> (candles[uc1].PosY * 8)) &&	
			   		((Y_pos_sprite - 3)	< (candles[uc1].PosY * 8)))	{			     
						candles[uc1].flame_enable = true;
						candles[uc1].flame_counter = 0;
						sprite_ani_max = ani_match_smoking_max; 
						sprite_ani_cnt = 0;
						memcpy_P( &sprite_animation[0], &ani_match_smoking[0], sizeof(sprite_animation) );	
						program_mode = PM_Smoking_Match;
						ucCNT = 0;

			   	}
			} 
			// inflame the tree during inactivity
			if ((GetTile((X_pos_sprite / 8) - 1, Y_pos_sprite / 8)) && 
			    (X_pos_sprite < 136) && (ucIDLE_TMR == 15)) {
				    SetSpriteVisibility(false);	
					burning_tree();
					set_PM_mode(PM_Intro); 
				}

			
		}

		// collision for base candle
		if (program_mode == PM_Match_wo_Flame) {
			// check x and y position of match
			if (((X_pos_sprite + 12) > (candles[0].PosX * 8)) &&	  
			   ((X_pos_sprite + 8)	< (candles[0].PosX * 8)) &&	
			   ((Y_pos_sprite + 4)	> (candles[0].PosY * 8)) &&	
			   ((Y_pos_sprite - 3)	< (candles[0].PosY * 8)))	{			     
				  	program_mode = PM_Match_Flame;
					sprite_ani_max = ani_match_burning_big_max; 
					sprite_ani_cnt = 0;
					memcpy_P( &sprite_animation[0], &ani_match_burning_big[0], sizeof(sprite_animation) );	
			   }
		}


		// all candles on - game finished?
		uc2 = 1;
		for(uc1 = 1; uc1 < 7; uc1++) {
			if (candles[uc1].flame_enable == false) {
				uc2 = 0;
				break;
			}
		}
		if (uc2 == 1) set_PM_mode(PM_Game_finish);
        break;

 	  case PM_Game_finish:
        // twinkle twinkle little star
		for(uc1 = 0; uc1 < star_coordinates_max; uc1++) {
		    sprite_ani_cnt = ani_blinking_star_max;
			for(uc2 = 0; uc2 <= ani_blinking_star_max; uc2++) {	
			    X_pos_sprite = star_coordinates[uc1][0];
			    Y_pos_sprite = star_coordinates[uc1][1];
				animate_sprite();	
				animate_candles();						
				WaitVsync(5);
			}
			WaitVsync(40);
		}
		if (check_highscore()) set_PM_mode(PM_HoF_edit);
		else set_PM_mode(PM_Intro);		
        break;

	  case PM_HoF_edit:				
		// cursor blinking
		WaitVsync(1);
		uc1++;
		if (uc1 >= 10) uc1 = 0;
		// proceed cursor position with left & right button
		if ((ibuttons & BTN_RIGHT) && !(ibuttons_old & BTN_RIGHT)) {
		  if (PosX < 7) {       	   
		  	show_highscore_char(PosY - 1, PosX, 0); 		 
		    PosX++; 			
          }
		}
		if ((ibuttons & BTN_LEFT) && !(ibuttons_old & BTN_LEFT)) {		 
 		  if (PosX) {
		    show_highscore_char(PosY - 1, PosX, 0); 
		    PosX--; 
          }
		}
		// chose character up & down button
		if ((ibuttons & BTN_UP) && !(ibuttons_old & BTN_UP)) {
		  edit_highscore_entry(PosY,PosX,BTN_UP); 
		}
		else if ((ibuttons & BTN_DOWN) && !(ibuttons_old & BTN_DOWN)) {		 
 		  edit_highscore_entry(PosY,PosX,BTN_DOWN); 
		}     
		// show cursor
		show_highscore_char(PosY - 1, PosX, uc1 > 4);

		// store new entry
		if (ibuttons & BTN_A)   
		{
		  // store new highscore 
		  EepromWriteBlock(&ebs);
		  //if (Music_on) fade_out_volume();
		  set_PM_mode(PM_Intro);
		}
		break;




	}
	

  }
  

} 


void set_PM_mode(u8 mode) {
// set parameters, tiles, background etc for choosed program mode
//u8 uc1, uc2;

			
	switch (mode)
	{

	  case	PM_Intro:

		SetSpriteVisibility(false);
		StopSong();

  		// init tile table
		SetTileTable(IntroTiles);
  		// init font table
		SetFontTilesIndex(INTROTILES_SIZE);
   		 
	    // cursor is invisible now	    
		ClearVram();

		DrawMap2(3,2,banner);

		Print(8,6,PSTR("FOR E/UZEBOX"));
		Print(6,8,PSTR("WWW.HWHARDSOFT.DE"));

		//Print(9,13,PSTR("PRESS START"));

		
		Print(4,19,PSTR("---= TOP BURNER =---"));
        view_highscore_entry(5,21,1,true);
        view_highscore_entry(5,23,2,true);
        view_highscore_entry(5,25,3,true);  
		StartSong(odufrohl);
		break;
		

	  case	PM_Game_finish:
		//draw star on the top of christmas tree
		DrawMap2(9,2,star);
  		MapSprite2(0,empty_sprite_big,0);

		sprite_ani_max = ani_blinking_star_max; 
		memcpy_P( &sprite_animation[0], &ani_blinking_star[0], sizeof(sprite_animation) );	
		StartSong(we_wish_you);					
		break;
	 

	  case	PM_Match_wo_Flame:

		// init tile table
  		SetTileTable(BGTiles);
		SetSpritesTileTable(SpriteTiles);
  
		// blank screen - no music
		StopSong();
        ClearVram();

		WaitVsync(2);

		// init variables
		stop_watch_tmr = 0;

		//draw christmas tree
		DrawMap2(2,4,tree);	
		
		//draw base candle	
		DrawMap2(24,20, candle);

		// init candle flames
		candles[0] = default_flame0; 	// flame of base candel - always on 
		candles[1] = default_flame1; 
		candles[2] = default_flame2; 
		candles[3] = default_flame3; 
		candles[4] = default_flame4; 
		candles[5] = default_flame5; 
		candles[6] = default_flame6; 


        //show sprite
		X_pos_sprite = 20*8;
		Y_pos_sprite = 23*8;
		sprite_ani_max = ani_match_without_flame_max; 
		sprite_ani_cnt = 0;
		memcpy_P( &sprite_animation[0], &ani_match_without_flame[0], sizeof(sprite_animation) );	
		MoveSprite(0,X_pos_sprite,Y_pos_sprite,2,1);
		SetSpriteVisibility(true);			
		StartSong(jbells);	
		break;

      case	PM_HoF_edit:
	    SetSpriteVisibility(false);	
  		// init tile table
		SetTileTable(IntroTiles);
  		// init font table
		SetFontTilesIndex(INTROTILES_SIZE);
 
	    PosY = check_highscore();
	    if (PosY == 2) copy_highsore(1,2);
	    if (PosY == 1) {
		  copy_highsore(1,2);
		  copy_highsore(0,1);
        }
		clear_highsore(PosY - 1);
		// reset cursor to left position
		PosX = 0;
		
		// prepare screen	
	    ClearVram();
	    Print(8,3,PSTR("CONGRATULATION"));	
		Print(8,5,PSTR("NEW HIGHSCORE!"));	
		Print(8,22,PSTR("ENTER YOUR NAME"));	
		Print(6,24,PSTR("AND PRESS BUTTON A"));	

        view_highscore_entry(6,12,1,!(PosY));
        view_highscore_entry(6,14,2,!(PosY));
        view_highscore_entry(6,16,3,!(PosY));  
		break;	

	}



	program_mode = mode;

}


/**** A N I M A T I O N S ***************************************************************/

void animate_sprite(void) {
// animate the sprite
  // increment the sprite animation picture
  if (sprite_ani_cnt < sprite_ani_max) sprite_ani_cnt++;
  else sprite_ani_cnt = 0;
  MapSprite2(0,(const char *)(sprite_animation[sprite_ani_cnt - 1]),0);
  MoveSprite(0,X_pos_sprite,Y_pos_sprite,2,1);
}


void animate_candles(void) {
// animate the candle flames
  u8 uc1,uc2;
  for(uc1 = 0; uc1 < 7; uc1++) {
	 if (candles[uc1].flame_enable) {
	   candles[uc1].flame_counter++;
	   uc2 = candles[uc1].flame_counter % 4;
	   if (uc2 == 3) uc2 = 0;
	   SetTile(candles[uc1].PosX,candles[uc1].PosY,uc2 + 1);	
	 } 
  }


}






/**** S T U F F ********************************************************************/

u8 GetTile(u8 x, u8 y)
// get background tile from vram
{

 return (vram[(y * 30) + x] - RAM_TILES_COUNT);

}



void burning_tree(void) {
unsigned char ucX, ucY;
	for(ucY=4;ucY<26;ucY++)
	for(ucX=2;ucX<20;ucX++) {
	  if(GetTile(ucX,ucY)) SetTile(ucX,ucY, ((ucX + ucY) % 3) + 1);
	  WaitVsync(1);
	}
}



/**
Print an unsigned char in decimal
@param x position of text output
@param y position of text output
@param leading zeros on/off
@return none
*/
void CandlePrintByte(int x,int y, unsigned char val,bool zeropad){
	unsigned char c,i;

	for(i=0;i<3;i++){
		c=val%10;
		if(val>0 || i==0){
			// SetFont(x--,y,c+CHAR_ZERO+RAM_TILES_COUNT);
			SetTile(x--, y, c + FONT_OFFSET);
		}else{
			if(zeropad){
				// SetFont(x--,y,CHAR_ZERO+RAM_TILES_COUNT);
				SetTile(x--, y, c + FONT_OFFSET);
			}else{
				// SetFont(x--,y,0+RAM_TILES_COUNT);
				SetTile(x--, y, 0 + FONT_OFFSET);
			}
		}
		val=val/10;
	}
		
}

/**
Print an unsigned int in decimal
@param x position of text output
@param y position of text output
@param leading zeros on/off
@param count of digits
@return none
*/
void CandlePrintInt(int x,int y, unsigned int val,bool zeropad, u8 digits){
	unsigned char c,i;

	for(i=0;i<digits;i++){
		c=val%10;
		if(val>0 || i==0){
			//SetFont(x--,y,c+CHAR_ZERO+RAM_TILES_COUNT);
			SetTile(x--, y, c + FONT_OFFSET);
		}else{
			if(zeropad){
				// SetFont(x--,y,CHAR_ZERO+RAM_TILES_COUNT);
				SetTile(x--, y, c + FONT_OFFSET);
			}else{
				// SetFont(x--,y,0+RAM_TILES_COUNT);
				SetTile(x--, y, 0 + FONT_OFFSET);
			}
		}
		val=val/10;
	}
		
}


/**
view the value of stop watch timer
@param x position of text output
@param y position of text output
@param leading zeros on/off
@param count of digits
@return none
*/
void view_stop_watch(int x,int y,int time_value){
unsigned int val;
	
	//display tenth of a second
	val = time_value%10;
	CandlePrintInt(x + 7,y,val * 10,true,2);
	
	//display colon 
	SetTile(x + 5, y, 10 + FONT_OFFSET);

	//display seconds
	val = time_value/10;
	val = val%60;
	CandlePrintInt(x + 4,y,val,true,2);

	//display colon 
	SetTile(x + 2, y, 10 + FONT_OFFSET);

	//display minutes
	val = time_value/600;
	val = val%60;
	CandlePrintInt(x + 1,y,val,true,2);
}


/**
view the value of stop watch timer outside game
@param x position of text output
@param y position of text output
@param leading zeros on/off
@param count of digits
@return none
*/
void view_stop_watch_title(int x,int y,int time_value){
unsigned int val;
	
	//display tenth of a second
	val = time_value%10;
	PrintInt(x + 7,y,val * 10,true);
	
	//display colon 
	Print(x + 5, y, PSTR(":"));

	//display seconds
	val = time_value/10;
	val = val%60;
	PrintInt(x + 4,y,val,true);

	//display colon 
	Print(x + 2, y, PSTR(":"));

	//display minutes
	val = time_value/600;
	val = val%60;
	PrintInt(x + 1,y,val,true);

	//delete leading 3 zeros of minutes
	Print(x - 3,y,PSTR("   "));

}


u8 check_highscore(void) {
// check the actual highsore
u8 a;
int i1;
   // read the eeprom block
  if (!isEepromFormatted() || EepromReadBlock(27, &ebs))
        return(0);   
  for(a=0; a<3; a++) {
    i1 = (ebs.data[(a * 10)+8] * 256) + ebs.data[(a * 10)+9];
    if (stop_watch_tmr < i1) return(a + 1);
  }

  // highscore is lower as saved highscores 
  return(0);
}



void copy_highsore(u8 entry_from, u8 entry_to) {
// copy a highscore entry to another slot
u8 a;
   // read the eeprom block
  for(a=0; a<10; a++) {
    ebs.data[(entry_to * 10) + a] = ebs.data[(entry_from * 10) + a];
  } 
}


void clear_highsore(u8 entry) {
// clear the name in actual entry and set the score to highscore
u8 a;
  // clear name 
  for(a=0; a<8; a++) {
    ebs.data[(entry * 10) + a] = 0x20;
  }   
  // set score
  ebs.data[(entry * 10) + 8] = stop_watch_tmr / 256;
  ebs.data[(entry * 10) + 9] = stop_watch_tmr % 256;
}



u8 set_def_highscore(void) {
// write the default highscore list in the EEPROM
  // entry 1
  ebs.data[0] = 'U';
  ebs.data[1] = 'Z';
  ebs.data[2] = 'E';
  ebs.data[3] = ' ';
  ebs.data[4] = ' ';
  ebs.data[5] = ' ';
  ebs.data[6] = ' ';
  ebs.data[7] = ' ';
  ebs.data[8] = 0x07;
  ebs.data[9] = 0x08;
  // entry 2
  ebs.data[10] = 'H';
  ebs.data[11] = 'A';
  ebs.data[12] = 'R';
  ebs.data[13] = 'T';
  ebs.data[14] = 'M';
  ebs.data[15] = 'U';
  ebs.data[16] = 'T';
  ebs.data[17] = ' ';
  ebs.data[18] = 0x09;
  ebs.data[19] = 0x60;
  // entry 3
  ebs.data[20] = 'C';
  ebs.data[21] = 'A';
  ebs.data[22] = 'R';
  ebs.data[23] = 'S';
  ebs.data[24] = 'T';
  ebs.data[25] = 'E';
  ebs.data[26] = 'N';
  ebs.data[27] = ' ';
  ebs.data[28] = 0x0B;
  ebs.data[29] = 0xB8;
  return(EepromWriteBlock(&ebs));
}


u8 view_highscore_entry(u8 x, u8 y, u8 entry, u8 load_data) {
// shows an entry of the higscore
u8 a,c;

  // read the eeprom block
  if (load_data)
  {
    if (!isEepromFormatted() || EepromReadBlock(27, &ebs))
        return(1);   
  }
  entry--;
  view_stop_watch_title(x + 10, y,(ebs.data[(entry * 10)+8] * 256) + ebs.data[(entry * 10)+9]);
  for(a = 0; a < 8;a++) {
	c = ebs.data[a + (entry * 10)];  
	PrintChar(x + a, y, c);  
  }
  return(0);
}



void edit_highscore_entry(u8 entry, u8 cursor_pos, u8 b_mode) {
// edit and view and char in the name of choosed entry    
entry--;
u8 c = ebs.data[(entry * 10) + cursor_pos];
  // proceed up & down button
  if (b_mode == BTN_UP) {
     c++;
     if (c > 'Z') c = ' '; 
     else if (c == '!') c = 'A';
  }
  if (b_mode == BTN_DOWN) {		 
     c--;      
     if (c == 0x1F) c = 'Z';
	 else if (c < 'A') c = ' ';
  }
  ebs.data[(entry * 10) + cursor_pos] = c;

}


void show_highscore_char(u8 entry, u8 position, u8 cursor_on) {
// shows a char of edited name
u8 c = ebs.data[(entry * 10) + position];
    if (cursor_on) PrintChar(6 + position, (entry * 2) + 12, '_');   // show '_'
    else if (c == ' ') PrintChar(6 + position, (entry * 2) + 12, ' ');	// space
    else PrintChar(6 + position, (entry * 2) + 12, c); 	
}




/**
copy a buffer into another buffer 
@param source buffer
@param target buffer
@param count of copied bytes
@return none
*/
void copy_buf(unsigned char *BUFA, unsigned char *BUFB, unsigned char ucANZ)
{
 for(;ucANZ>0 ; ucANZ--)
 {
  *(BUFB++) = *(BUFA++);
 }   
}


/**
fill a buffer 
@param target buffer
@param byte to fill
@param count of copied bytes
@return none
*/
void fill_buf(u8 *BUFA, u8 content, u8 ucANZ)
{
 for(;ucANZ>0 ; ucANZ--)
 {
  *(BUFA++) = content;
 }   
}



