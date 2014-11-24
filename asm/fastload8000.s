;fastload8000.s

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

	lda	ld_beg		; load begin LSB location
	sta	store+1		; store it
	lda	ld_beg+1	; load begin MSB location
	sta	store+2		; store it

	ldx	#0		; set X to 0
	lda	#1		; set A to 0 

nsync:	bit	tapein		; 4 cycles, sync bit ; first pulse
	bpl	nsync		; 2 + 1 cycles

main:	ldy	#0		; 2 set Y to 0 

psync:	bit	tapein		; 
	bmi	psync

ploop:	iny			; 2 cycles
	bit	tapein		; 4 cycles
	bpl	ploop		; 2 +1 if branch, +1 if in another page
				; total ~9 cycles

	cpy	#$40		; 2 cycles if Y - $40 > 0 endcode (770Hz)
	bpl	endcode		; 2(3)

	cpy	#$15		; 2 cycles if Y - $15 > 0 main (2000Hz)
	bpl	main		; 2(3)

	cpy	#$07		; 2, if Y<, then clear carry, if Y>= set carry
store:	rol	store+1,x	; 7, roll carry bit into store
	ldy	#0		; 2
	asl			; 2 A*=2
	bne	main		; 2(3)
	lda	#1		; 2
	inx			; 2 cycles
	bne	main		; 2(3)
	inc	store+2		; 6 cycles
	jmp	main		; 3 cycles
				; 34 subtotal max
				; 36 subtotal max
endcode:  
	txa			; write end of file location + 1
	clc
	adc	store+1
	sta	store+1
	bcc	endcheck	; LSB didn't roll over to zero
	inc	store+2		; did roll over to zero, inc MSB
endcheck:			; check for match of expected length
	lda	ld_end
	cmp	store+1
	bne	error
	lda	ld_end+1
	cmp	store+2
	bne	error
sumcheck:
	lda	#0
	sta	pointer
	lda	ld_beg+1
	sta	pointer+1
	lda	#$ff		; init checksum
	ldy	ld_beg
sumloop:
	eor	(pointer),y

	;last page?

	ldx	pointer+1
	cpx	ld_end+1
	beq	last
	iny
	bne	sumloop
	inc	pointer+1
	bne	sumloop
last:
	iny
	cpy	ld_end
	bcc	sumloop

;	sty	$01
	sta	chksum
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

