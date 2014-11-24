;diskload9600.s

org	=	$9000		; should be $9000
cout	=	$FDED		; character out sub
crout	=	$FD8E		; CR out sub
prbyte	=	$FDDA 		; print byte in hex
tapein	=	$C060		; read tape interface
warm	=	$FF69		; back to monitor
clear	=	$FC58		; clear screen
endbas	=	$80C
target	=	$1000

; zero page parameters

begload	=	$00		; begin load location LSB/MSB
endload	=	$02		; end load location LSB/MSB
chksum	=	$04		; checksum location
pointer	=	$0C		; LSB/MSB pointer

start:
        .org	endbas
move:				; end of BASIC, move code to readtape addr
	ldx	#0
move1:
	lda	moved,x
        sta	readtape,x
;	lda	moved+256,x
;	sta	readtape+256,x
	inx
	bne	move1
phase1:
	jsr	crout		; print LOADING...
	lda	#<loadm
	ldy	#>loadm
	jsr	print
				; diskload2 ORG
	lda	#$D0		; store begin location LSB
	sta	begload
	lda	#$96		; store begin location MSB
	sta	begload+1
				; end of DOS + 1 for comparison
	lda	#$00		; store end location LSB
	sta	endload
	lda	#$C0		; store end location MSB
	sta	endload+1

	jsr	readtape	; get the code
	jmp	$9700		; run it
loadm:
	.byte	"LOADING INSTA-DISK, ETA "
loadsec:			; 10 bytes for "XX SEC. ",$00
	.byte	0,0,0,0,0,0,0,0,0,0
moved:
	.org	org		; $9000
readtape:
	lda	begload		; load begin LSB location
	sta	store+1		; store it
	lda	begload+1	; load begin MSB location
	sta	store+2		; store it

	lda	#$ff		; init checksum
	sta	chksum

wait:	bit	tapein
	bpl	wait
waithi:	bit	tapein		; Wait for input to go high.
	bmi	waithi
pre:	lda	#1		; Load sentinel bit
	ldx	#0		; Clear data index
	clc			; Clear carry (byte complete flag)
data:	bcc	waitlo		; Skip if byte not complete
store:	sta	target,x	; Store data byte

	eor	chksum		; compute checksum
	sta	chksum		; store checksum

	lda	#1		; Re-load sentinel bit
waitlo:	bit	tapein		; Wait for input to go low
	bpl	waitlo
	bcc	poll9		; Poll at +9 cycles if no store	
	inx			; Stored, so increment data index
	bne	poll13		; Poll at +13 cycles if no carry
	inc	store+2		; Increment data page
	bne	poll21		; (always) poll at +21 cycles

one:	sec			; one bit detected
	rol			;  shift it into A
	jmp	data		;   and go handle data (C = sentinel)

zero:	clc			; zero bit detected
	rol			;  shift it into A
	jmp	data		;   and go handle data (C = sentinel)

poll9:	bit	tapein
	bpl	zero		; zero bit (short)(9-15 cycles)
poll13:	bit	tapein		; (2 cycles early if branched here)
	bpl	zero		; zero bit (15-21 cycles)
poll21:	bit	tapein
	bpl	zero		; zero bit (21-27 cycles)
	bit	tapein
	bpl	zero		; zero bit (27-33 cycles)
	bit	tapein
	bpl	zero		; zero bit (33-39 cycles)
	bit	tapein
	bpl	zero		; zero bit (39-45 cycles)
	bit	tapein
	bpl	zero		; zero bit (45-51 cycles)
	bit	tapein
	bpl	zero		; zero bit (51-57 cycles)
	bit	tapein
	bpl	zero		; one bit (57-63 cycles)
	bit	tapein
	bpl	one		; one bit (63-69 cycles)
	bit	tapein
	bpl	one		; one bit (69-75 cycles)
	bit	tapein
	bpl	one		; one bit (75-81 cycles)
	bit	tapein
	bpl	one		; one pulse (81-87 cycles)
	bit	tapein
	bpl	pre		; pre pulse (87-93 cycles)
	bit	tapein
	bpl	pre		; pre pulse (93-99 cycles)
	bit	tapein
	bpl	pre		; pre pulse (99-105 cycles)

				; low freq signals end of data
endcode:  
	txa			; write end of file location + 1
	clc
	adc	store+1
	sta	store+1
	bcc	endcheck	; LSB didn't roll over to zero
	inc	store+2		; did roll over to zero, inc MSB
endcheck:			; check for match of expected length
	lda	endload
	cmp	store+1
	bne	error
	lda	endload+1
	cmp	store+2
	bne	error
	jsr	ok
sumcheck:
	jsr	crout
	lda	#<chkm
	ldy	#>chkm
	jsr	print
	lda	chksum
	bne	error
	jmp	ok		; return to caller
error:
	lda	#<errm
	ldy	#>errm
	jsr	print
	jmp	warm	
ok:
	lda	#<okm
	ldy	#>okm
print:
	sta	pointer
	sty	pointer+1
	ldy	#0
	lda	(pointer),y	; load initial char
print1:	ora	#$80
	jsr	cout
	iny
	lda	(pointer),y
	bne	print1
	rts

chkm:	.asciiz	"CHKSUM "
okm:	.asciiz	"OK"
errm:	.asciiz	"ERROR"
end:
