/**
 * This file contains all the ui routines.
 */


#include <ncurses.h>
#include "gamepieces.h"
#include "connection.h"
#include "screen.h"

#define MAX_NAME 100

extern Ship Shipset[];
char name[MAX_NAME];

WINDOW *player_win;
WINDOW *opponent_win;
WINDOW *status_win;

void place_hit_or_mis(WINDOW * win,int mesg, int x, int y) {
  //-2game -1 hit sink 1hit  0miss
  //deal with hits first

  if ((mesg == -2) || (mesg == -1) || (mesg == 1)) {
    wattron(win,COLOR_PAIR(4));
    mvwprintw(win, y+2, x*2+3,"#");
    wattroff(win,COLOR_PAIR(4));
    wrefresh(win);
  }

  else { // miss
    wattron(win,COLOR_PAIR(3));
    mvwprintw(win, y+2, x*2+3,"@");
    wattroff(win,COLOR_PAIR(3));
    wrefresh(win);
  }

}

/**
 * Documentation here
 */
void show_battlefield(const Board *board)
{
  printw("Hello, world!");
  refresh();
  getch();
  return;
}

/**
 * Documentation here
 */
void main_menu()
{
  int user_mode;
  Player *player;
  title_screen();
  printw("Enter your name: ");
  getstr(name);
  printw("\nEnter 0 for server mode, 1 for client mode: ");
  scanw("%d", &user_mode);
  player = create_player(name, user_mode);

  printw("\ninitializing the game...\n");
  refresh();
  init_game(user_mode);
}

void return_cords(int * x, int * y) {


  struct player_pos_ player_pos;
  int startx, starty,height, width;
  
  player_pos.x = 0;
  player_pos.y = 0;

  keypad(stdscr, TRUE);
  curs_set(1); // make cursor visible
  height = 3+BOARD_SIZE; 
  width = 14+BOARD_SIZE; 
  starty = (LINES - height) / 2;  
  startx = (COLS - width) / 2;   

  int playerx = 3+startx+20;
  int playery = 2+starty;
  move(playery,playerx);

  int ch;
  while((ch = getch())) 
    {      
      switch(ch) 
	{       case KEY_LEFT:
	    if (playerx > 3+startx+20) {
	      playerx -=2;
	      player_pos.x--;
	      move(playery, playerx);
	      break;
	    }
	    break;
	case KEY_RIGHT:
	  if (playerx < -3+startx+20+width) {
	    playerx +=2;
	    player_pos.x++;
	    move(playery, playerx);
	    break; 
	  }
	  break;
	case KEY_UP:
	  if (playery > 2+starty) {
	    --playery;
	    --player_pos.y;
	    move(playery, playerx);
	    break;
	  }
	  break;
	case KEY_DOWN:
	  if (playery < starty+height-2) {
	    ++playery;
	    ++player_pos.y;
	    move(playery, playerx);
	    break;  
	  }
	  break;
	case 10:
	  *x = player_pos.x;
	  *y = player_pos.y;
	  return;
	  break;
	  break;  
			  
	case KEY_ENTER:
	  *x = player_pos.x;
	  *y = player_pos.y;
	  return;
	  break;
	  break;
		
	}
    } 
}

void display_boards(void) {
  int startx, starty, width, height; 
  int stat_width, stat_height;

  char players_grid[BOARD_SIZE][BOARD_SIZE];
    
  int x,y,res;
  int f, h = 0;
  char t;
  int i;
  stat_height= 5;
  stat_width=50;

  cbreak();
  noecho();                       
                                        
  keypad(stdscr, TRUE);            
  height = 3+BOARD_SIZE; 
  width = 14+BOARD_SIZE; 
  starty = (LINES - height) / 2;  
  startx = (COLS - width) / 2;    
  clear();
  refresh(); 
    
  player_win = newwin(height, width, starty, startx+20); 
  box(player_win, 0, 0);
  wrefresh(player_win);

  opponent_win = newwin(height, width, starty, startx-20);
  box(opponent_win, 0, 0);
  wrefresh(opponent_win);

  status_win = newwin(stat_height, stat_width, starty+13, startx-20);

  create_grid(players_grid);

  start_color();
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  clear();
  refresh();

  mvprintw(starty-1, startx-15, "Your ships");
  mvwprintw(opponent_win, 1,1,"  A B C D E F G H I J");
  wattron(opponent_win,COLOR_PAIR(2));
  mvwprintw(opponent_win, 1, 1, " ");
  wattroff(opponent_win,COLOR_PAIR(2));

  for (h = 0; h<BOARD_SIZE; h++) {
    mvwprintw(opponent_win, 2+h, 1,"%d|", h);
    for (f =0; f<BOARD_SIZE; f++) {
      t = players_grid[h][f];
      if (t == '*') {
	wattron(opponent_win,COLOR_PAIR(2));
	mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);
	wattroff(opponent_win,COLOR_PAIR(2));
      }
      else {mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);}
      mvwprintw(opponent_win, 2+h,4+f*2, "|");
    }
  }
  wrefresh(opponent_win);

  mvprintw(starty-1, startx+21, "Hit or miss ships");
  mvwprintw(player_win,1,1,"  A B C D E F G H I J");

  for (i=0;i<BOARD_SIZE;i++) {
    mvwprintw(player_win,i+2,1,"%d|_|_|_|_|_|_|_|_|_|_|",i+0);
  }
  refresh();
  wrefresh(player_win);
  wrefresh(status_win); 
    
  attroff(A_UNDERLINE);

}

/**
 * This function gets called from within init_game
 * It is called repeatedly if we are the server, else
 * it is just called once.
 */
void do_gameplay(const int sock, int fire)
{
  int x,y,res,win_status=0;

  initShips();
  display_boards();

  Ship sh;
  do {
    if (fire == 1) { /*you're the attacker*/
      mvwprintw(status_win,1,1,"It's your turn!                    ");
      wrefresh(status_win);
            
      fire = 0;
      return_cords(&x, &y);
      res = do_fire(sock, x, y);
      place_hit_or_mis(player_win,res, x, y);
      switch (res) {
      case 0:
	mvwprintw(status_win,2,1,"Missed!                                    ");
	wrefresh(status_win);
	break;
      case 1:
	mvwprintw(status_win,2,1,"You hit them!                              ");
	wrefresh(status_win);
	break;
      case -1:
	mvwprintw(status_win,2,1,"You sunk them!                             ");
	wrefresh(status_win);
	break;
      case -2:
	win_status = 1;
	mvwprintw(status_win,2,1,"Game over!                        ");
	wrefresh(status_win);
	fire = -1;
	break;
      }

    } else { /*you're the defender*/
      keypad(stdscr, FALSE);
      curs_set(0); // Set cursor invisible
      mvwprintw(status_win,1,1,"Waiting for other player to fire...");
      wrefresh(status_win);
      res = do_receive(sock);
      refresh();
      if (res == 0) {
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They missed!                                ");
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      } else if (res < 0) { //negative res indicates sunken ship
	sh = getShipById(-1*res);
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They sunk your %s!               ", sh.name);
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      } else if (res==100);//do nothing...the game is over
      else {
	sh = getShipById(res);
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They hit your %s!                ", sh.name);
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      }
      mvwprintw(status_win,1,1,"It's your turn!                               ");
      fire = (check_game_over() == 1) ? -1 : 1;
      refresh();
    }
  } while(fire > -1);
  clear();
  refresh();
  printw("Game over, %s, you %s!\nthanks for playing!!!\n\npress any key to continue...\n", name, (win_status) ? "win" : "lose");
  refresh();
  getch();
}


void title_screen()
{
  printw(\
	 "                                     # #  ( )\n" \
	 "                                  ___#_#___|__\n" \
	 "                              _  |____________|  _\n" \
	 "                       _=====| | |            | | |==== _\n" \
	 "                 =====| |.---------------------------. | |====\n" \
	 "   <--------------------'   .  .  .  .  .  .  .  .   '--------------/\n" \
	 "     \\                                                             /\n" \
	 "      \\_______________________________________________WWS_________/\n" \
	 "  wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n" \
	 "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n" \
	 "   wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww \n");
  printw("\n\nPress any key to continue...\n");
  getch();
}