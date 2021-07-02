// Bitte fuegen Sie ihre Loesung im Abschnitt
//
// LOESUNG:
//
// ein!

.globl main

.data
// Sie duerfen die unten stehenden Werte veraendern, um ihre Implementeriung zu testen!
q:	.word	12 15 21 0 3 27 3 7 9 19 21 18 4 23 6 24 24 12 26 1 6 7 23 14 24 17 5 25 13 8 9 20 19 16 19 5 30 15 15 0 18 3 24 17 19 29 19 19 14 7 0 1 9 25 31 0 31 10 20 23 3 30 11 18 23 28 2 00 4 21 5 6 8 20 17 15 4 9 10 26 31 24 1 1 7 9 25 3 6 23 11 14 18 27 0 14 3 21 12 25 10 20 11 4 6 4 15 31 20 3 12 4 20 8 14 15 20 3 26 23 15 13 21 21 16 17 5 9 3 0 31 5 30 0 17 18 4 2 16 29 3 2 10 13 16 7 21 9 31 0 10 18 11 26 31 23 27 2 25 2 30 3 30 27 3 18 14 3 20 17 18 27 14 9 26 1 4 10 22 11 8 11 2 19 16 0 22 0 6 19 14 10 19 24 28 8 13 30 24 29 2 3 30 2 11 13 16 8 8 19 31 8 26 2 24 20 3 12 29 14 0 4 3 13 11 28 22 13 25 13 11 24 16 24 29 21 14 25 16 28 25 22 21 29 19 1 8 0 4 27 27 25 6 21 31 13 7 24 24 15 9 18 8 22 15 11 6 15 29 25 1 31 22 12 24 29 24 3 18 15 3 10 12 6 3 22 30 29 5 23 11 0 11 8 20 10 22 11 5 29 28 15 8 2 27 19 25 23 21 19 14 20 21 29 3 3 7 9 9
seed: 	.word 	0
key: 	.word	3

//############################################
// Keys mit ihren korrespondierenden Hashwerten fuer den Seed 0
// Key	- > Hashwert 
//  0	- >	0
//  1	- > 	12
//  2 	- >	15 
//  3 	- > 	3
//  4 	- >	21
//  5 	- > 	25 
//  6	- > 	26 
//  7 	- > 	22
// ############################################


.text

// ############################################
// LOESUNG:
// X0: Startadresse des Arrays q
// X1: Seed
// X2: Schluessel
// X3: Hash-Wert

// X9: value of checked bit of X2
// X10: loop counter
// X11: temporary
// X12: one to compare bits of value
// X13: temporary
// X14: current address in X0

hash:
	ADD X9, XZR, XZR
	ADD X3, XZR, XZR
	ADD X10, XZR, XZR
	ADDI X12, XZR, #1
	ADD X14, X0, XZR
	ADD X15, X1, XZR
	ADDI X11, XZR, #32
	LSL X11, X11, #2
	MUL X13, X15, X11
	ADD X14, X14, X13
loop:
	AND X9, X2, X12
	CBNZ X9, one
continue:
	ADDI X10, X10, #1
	LSL X12, X12, #1
	SUBIS X11, X10, #32
	ADDI X14, X14, #4
	B.MI loop
	ADD X3, X3, XZR
	BR X30

one:
	
	LDUR X13, [X14, 0]
	EOR X3, X3, X13
	ADD X9, XZR, XZR
	B continue
//
// Ende der Loesung
// ############################################


main: 	
	// Sichern der Ruecksprungadresse
	SUBI X28, X28, #4
	STUR X30, [X28, #0]

	// Laden der Argumente
	LDA X0, q
	LDA X1, seed
	LDUR X1, [X1, #0]
	LDA X2, key
	LDUR X2, [X2, #0]
	// Aufruf der Unterfunktion
	BL hash

	// Dummy-Instruktion 
	ADDI X15, XZR, #3
	
	// Wiederherstellen der Ruecksprungadresse
	LDUR X30, [X28, #0]
	ADDI X28, X28, #4
