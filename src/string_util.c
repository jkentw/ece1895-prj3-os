/* J. Kent Wirant
 * 10 Dec. 2022
 * ECE 1895 - Project 3
 * string_util.c
 * Description: Custom string functions somewhat resembling those in
 *   ANSI C string library.
 */

#include "string_util.h"

/* Just like strncat, but last character in dest string is always 
 * null-terimated, and no null-padding is performed. The length of the
 * destination string (including the null character) is at most bufsize
 * characters.
 */
//char *strncat_safe(char *dest, const char *src, size_t n, size_t bufsize);

/* Just like strncpy, but last character in dest string is always 
 * null-terimated, and no null-padding is performed. The length of the
 * destination string (including the null character) is at most bufsize
 * characters.
 */
//char *strncpy_safe(char *dest, const char *src, size_t n, size_t bufsize);

//same as ANSI C definition
int strcmp(const char *s1, const char *s2) {
	for(int i = 0; (s1[i] == s2[i]) && (s1[i] != 0); i++);
	return *s1 - *s2;
}

//same as ANSI C definition
int strncmp(const char *s1, const char *s2, size_t n) {
	for(int i = 0; (i < n) && (s1[i] == s2[i]) && (s1[i] != 0); i++);
	return *s1 - *s2;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) len++;
	return len;
}
