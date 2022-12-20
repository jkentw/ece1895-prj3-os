/* J. Kent Wirant
 * 19 Dec. 2022
 * ECE 1895 - Project 3
 * text_util.h
 * Description:
 *   Provides text utility functions, including string manipulation
 *   and display.
 */
 
#ifndef TEXT_UTIL_H
#define TEXT_UTIL_H

static const char NUM_COLS = 80;
static const char NUM_ROWS = 25;

static const char COLOR_BLACK 			=  0;
static const char COLOR_BLUE			=  1;
static const char COLOR_GREEN 			=  2;
static const char COLOR_CYAN 			=  3;
static const char COLOR_RED 			=  4;
static const char COLOR_MAGENTA			=  5;
static const char COLOR_BROWN 			=  6;
static const char COLOR_LIGHT_GRAY		=  7;
static const char COLOR_DARK_GRAY		=  8;
static const char COLOR_LIGHT_BLUE		=  9;
static const char COLOR_LIGHT_GREEN		= 10;
static const char COLOR_LIGHT_CYAN		= 11;
static const char COLOR_LIGHT_RED		= 12;
static const char COLOR_LIGHT_MAGENTA	= 13;
static const char COLOR_YELLOW			= 14;
static const char COLOR_WHITE			= 15;

void setCursorPosition(char row, char col);
void setTextColor(char foreground, char background);
void printRaw(const char *str);
void highlight(char row, char col);

void clearScreen(void);
//void printf(const char *fmt, ...);

//char * itoa(int value, char *buf, int radix);
//int atoi(const char *str);

#endif
