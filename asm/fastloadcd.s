;fastloadcd.s

org	=	$BE80		; should be $BE80
cout	=	$FDED		; character out sub
crout	=	$FD8E		; CR out sub
prbyte	=	$FDDA 
warm	=	$FF69		; back to monitor
tapein	=	$C060
pointer	=	$06
endbas	=	$80C
;target	=	$1000
target	=	$801
chksum	=	$00
inflate	=	$BA00
inf_zp	=	$0

start:
        .org	endbas
move:
	ldx	#0
move1:	lda	moved,x
        sta	fast,x
	inx
	bne	move1		; move 256 bytes
;	ldx	#0
move2:	lda	moved+256,x
	sta	fast+256,x
	inx
	bpl	move2		; only 128 bytes to move
	jmp	fast
moved:
	.org	org
fast:
	lda	#<loadm
	ldy	#>loadm
	jsr	print

	lda	#$ff
	sta	chksum

	lda	ld_beg		; setup point to target
	sta	store+1
	lda	ld_beg+1
	sta	store+2

wait:	bit	tapein
	bpl	wait
waithi:	bit	tapein		; Wait for input to go high.
	bmi	waithi
pre:	lda	#1		; Load sentinel bit
	ldx	#0		; Clear data index
	clc			; Clear carry (byte complete flag)
data:	bcc	waitlo		; Skip if byte not complete
store:	sta	store,x		; Store data byte

	eor	chksum
	sta	chksum

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
	bpl	one		; one pulse (87-93 cycles)
	bit	tapein
	bpl	pre		; pre pulse (93-99 cycles)
	bit	tapein
	bpl	pre		; pre pulse (99-105 cycles)
	bit	tapein
	bpl	pre		; pre pulse (99-111 cycles)

endcode:  
	txa			; write end of file location + 1
	clc
	adc	store+1
	sta	store+1
	bcc	endcheck
	inc	store+2
endcheck:
	lda	ld_end
	cmp	store+1
	bne	error
	lda	ld_end+1
	cmp	store+2
	bne	error
sumcheck:
	lda	chksum
	bne	sumerror
	lda	inf_flag	; if inf_flag = 0 runit
	beq	runit
inf:
	jsr	crout
	lda	#<infm
	ldy	#>infm
	jsr	print

	lda	inf_src		;src lsb
	sta	inf_zp+0
	lda	inf_src+1	;src msb
	sta	inf_zp+1
	lda	inf_dst		;dst lsb
	sta	inf_zp+2
	lda	inf_dst+1	;dst msb
	sta	inf_zp+3

	jsr	inflate

	lda	inf_end		;dst end +1 lsb
	cmp	inf_zp+2
	bne	error
	lda	inf_end+1	;dst end +1 msb
	cmp	inf_zp+3
	bne	error
runit:
	lda	warm_flag	; if warm_flag = 1 warm boot
	bne	warmit
	jmp	(runcode)
warmit:
	jmp	warm		; run it
sumerror:
	jsr	crout
	lda	#<chkm
	ldy	#>chkm
	jsr	print
error:
	lda	#<errm
	ldy	#>errm
	jsr	print
	jmp	warm	
print:
	sta	pointer
	sty	pointer+1
	ldy	#0
	lda	(pointer),y		; load initial char
print1:	ora	#$80
	jsr	cout
	iny
	lda	(pointer),y
	bne	print1
	rts
chkm:	.asciiz	"CHKSUM "
errm:	.asciiz	"ERROR"
infm:	.asciiz	"INFLATING "
ld_beg:
	.org	*+2
ld_end:
	.org	*+2
inf_src:
	.org	*+2
runcode:
inf_dst:
	.org	*+2
inf_end:
	.org	*+2
inf_flag:
	.org	*+1
warm_flag:
	.org	*+1
loadm:	
	;.asciiz	"LOADING "

end:

