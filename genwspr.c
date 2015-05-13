/*
 * Here is some C code to generate "Type 1" WSPR messages, that
 * include a single common callsign (no prefixes) and 4 character
 * Maidenhead gridsquare location, as well as transmitted power.
 * Some attempt has been made to minimize the storage used, so that
 * this can compactly be used on small microcontrollers.
 * 
 * Very little error checking is done on this, so you better make sure
 * that the callsign and gridsquare are of the appropriate form.
 *
 * Callsigns must be 2x3, 1x3, 2x1, or 1x2 for the purposes of this 
 * code.
 *
 * Written by Mark VandeWettering K6HX
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

const unsigned char sync[] = {
    1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0,
    1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1,
    1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
    0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1,
    1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0
} ;

const unsigned char rdx[] = {
    0, 128, 64, 32, 160, 96, 16, 144, 80, 48, 112, 8, 136, 72, 40, 104, 24,
    152, 88, 56, 120, 4, 132, 68, 36, 100, 20, 148, 84, 52, 116, 12, 140,
    76, 44, 108, 28, 156, 92, 60, 124, 2, 130, 66, 34, 98, 18, 146, 82, 50,
    114, 10, 138, 74, 42, 106, 26, 154, 90, 58, 122, 6, 134, 70, 38, 102,
    22, 150, 86, 54, 118, 14, 142, 78, 46, 110, 30, 158, 94, 62, 126, 1,
    129, 65, 33, 161, 97, 17, 145, 81, 49, 113, 9, 137, 73, 41, 105, 25,
    153, 89, 57, 121, 5, 133, 69, 37, 101, 21, 149, 85, 53, 117, 13, 141,
    77, 45, 109, 29, 157, 93, 61, 125, 3, 131, 67, 35, 99, 19, 147, 83, 51,
    115, 11, 139, 75, 43, 107, 27, 155, 91, 59, 123, 7, 135, 71, 39, 103,
    23, 151, 87, 55, 119, 15, 143, 79, 47, 111, 31, 159, 95, 63, 127 
} ;

char msg[162] ;

int 
chval1(int ch)
{
    if (isdigit(ch)) return ch - '0' ;
    if (isalpha(ch)) return 10 + toupper(ch) - 'A' ;
    if (ch == ' ') return 36 ;
}

int 
chval2(int ch)
{
    if (isalpha(ch)) return toupper(ch) - 'A' ;
    if (ch == ' ') return 26 ;
}


int
encodecallsign(const char *callsign)
{
    /* find the first digit... */
    int i, rc ;
    char call[6] ;

    for (i=0; i<6; i++) call[i] = ' ' ;

    if (isdigit(callsign[1])) {
	/* 1x callsigns... */
	for (i=0; i<strlen(callsign); i++)
	   call[1+i] = callsign[i] ;
    } else if (isdigit(callsign[2])) {
	/* 2x callsigns... */
	for (i=0; i<strlen(callsign); i++)
	   call[i] = callsign[i] ;
    } else {
	return 0 ;
    }

    rc  = chval1(call[0]) ; rc *= 36 ; 
    rc += chval1(call[1]) ; rc *= 10 ;
    rc += chval1(call[2]) ; rc *= 27 ;
    rc += chval2(call[3]) ; rc *= 27 ;
    rc += chval2(call[4]) ; rc *= 27 ;
    rc += chval2(call[5]) ;

    return rc ;
}

int 
encodegrid(const char *grid)
{
    int rc ;

    rc = (179 - 10 * (grid[0]-'A') - (grid[2] - '0')) * 180
	 + (10 * (grid[1]-'A')) + (grid[3] - '0') ;

    return rc ;
}

int
encodepower(int p)
{
    return p + 64 ;
}

int 
parity(unsigned int x)
{
    unsigned int sx = x ;
    int even = 0 ;
    while (x) {
	even = 1-even ;
	x = x & (x - 1) ;
    }
    return even ;
}

void
genmsg(const char *call, const char *grid, const int power)
{
    int c = encodecallsign(call) ;
    int g = encodegrid(grid) ;
    int p = encodepower(power) ;
    int i, mp = 0 ;
    unsigned int acc = 0;

    for (i=0; i<162; i++)
	msg[i] = sync[i] ;

    for (i=27; i>=0; i--) {		/* encode the callsign, 28 bits */
	acc <<= 1 ;
	if (c & (1<<i)) acc |= 1 ;
	msg[rdx[mp++]] += 2*parity(acc & 0xf2d05351L) ;
	msg[rdx[mp++]] += 2*parity(acc & 0xe4613c47L) ;
    }

    for (i=14; i>=0; i--) {		/* encode the grid, 15 bits */
	acc <<= 1 ;
	if (g & (1<<i)) acc |= 1 ;
	msg[rdx[mp++]] += 2*parity(acc & 0xf2d05351L) ;
	msg[rdx[mp++]] += 2*parity(acc & 0xe4613c47L) ;
    }

    for (i=6; i>=0; i--) {		/* encode the power, 7 bits */
	acc <<= 1 ;
	if (p & (1<<i)) acc |= 1 ;
	msg[rdx[mp++]] += 2*parity(acc & 0xf2d05351L) ;
	msg[rdx[mp++]] += 2*parity(acc & 0xe4613c47L) ;
    }

    for (i=30; i>=0; i--) {		/* pad with 31 zero bits */
	acc <<= 1 ;
	msg[rdx[mp++]] += 2*parity(acc & 0xf2d05351L) ;
	msg[rdx[mp++]] += 2*parity(acc & 0xe4613c47L) ;
    }
}

int
main(int argc, char *argv[])
{
    int i, j ;
    genmsg(argv[1], argv[2], atoi(argv[3])) ;

    /* dump the tone sequence */
    for (i=0; i<162; i += 18) {
	for (j=0; j<18; j++) {
	    printf("%d", msg[i+j]) ;
	    if (j < 17) printf(" ") ;
	}
	printf("\n") ;
    }
    return 0 ;
}
