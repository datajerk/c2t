;diskload2.s

; apple vectors

cout	=	$FDED		; character out sub
crout	=	$FD8E		; CR out sub
prbyte	=	$FDDA 		; print byte in hex
tapein	=	$C060		; read tape interface
warm	=	$FF69		; back to monitor
clear	=	$FC58		; clear screen
movecur	=	$FB5B		; move cursor to ch,a
dos	=	$9D84
asrom	=	$9D72
locrpl	=	$3E3		; locate RWTS paramlist jsr
rwts	=	$3D9		; RWTS jsr
cleos	=	$FC42		; clear to end of screen
init	=	$A54F
motoroff=	$C088		; Turn drive motor off
motoron	=	$C089		; Turn drive motor on
reboot	=	$FAA6		; reboot machine
bell	=	$FBDD		; ding
rdkey	=	$FD0C		; read key

; my vectors

;print	=	$90CE		; from diskload.s
readtape=	$9000
inflate	=	$9B00

; zero page parameters

begload	=	$00		; begin load location LSB/MSB
endload	=	$02		; end load location LSB/MSB
chksum	=	$04		; checksum location
secnum	=	$05		; loop var
trknum	=	$06		; loop var
segcnt	=	$07		; loop var
buffer	=	$08		; MSB of RWTS buffer
trkcnt	=	$09		; track counter (0-6)
pointer	=	$0A		; pointer LSB/MSB
prtptr	=	$0C		; pointer LSB/MSB
fmptr	=	$0E		; file manager pointer
inf_zp	=	$10		; inflate vars (10)
temp	=	$1E		; temp var
ch	=	$24		; cursor horizontal
preg	=	$48		; mon p reg

; other vars

org	=	$9700		; should be $9700
invsp	=	$60		; inverse space for draw
data	=	$1000		; 7 track dump from inflate
boot1o	=	$96D0		; tape loaded boot 1 location
boot1	=	$3D0		; target boot 1 location
cmpbuf	=	$9200		; buffer for sector check
count	=	$900

        .org	org

	ldx	#0		; move 9cd0 to 3d0
move1:
	lda	boot1o,x
	sta	boot1,x
	inx
	cpx	#48
	bne	move1		; branch on positive (0-127)
patch:
	lda	#$B3		; hack since chksum could not be written to C000
	sta	$BFFF		; chksum was written do BFFF
start:
	jsr	clear		; clear screen
	lda	#<title		; print title
	ldy	#>title
	jsr	inv
				; TRACK
	lda	#19		; col 20
	sta	ch
	lda	#0		; row 0
	jsr	movecur
	lda	#<track		; print track
	ldy	#>track
	jsr	print

	lda	#<header	; print header
	ldy	#>header
	jsr	print
	ldx	#35		; length of line
	jsr	line

	lda	#<left		; print left side of grid
	ldy	#>left
	jsr	print

setupiob:
	jsr	locrpl		; locate rwts paramlist
	sty	pointer		; and save pointer
	sta	pointer+1

	lda	#1		; table type
	ldy	#0		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#6 * 16		; slot 6
	ldy	#1		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#1		; drive number
	ldy	#2		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#254		; volume number
	ldy	#3		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

format:				; format the diskette
	lda	infdata+20	; check noformat flag
	bne	endformat	; if not 0 jump to endformat

	jsr	status
	lda	#<formatm	; print formatting
	ldy	#>formatm
	jsr	print

;;; RWTS format (issues)
;	lda	#4		; format(4) command
;	ldy	#$0C		; offset in RWTS
;	sta	(pointer),y	; write it to RWTS

;	jsr	locrpl		; locate rwts paramlist
;	jsr	rwts		; do it!
;	bcs	formaterror
;	lda	#0
;	sta	preg		; fix p reg so mon is happy
;	jmp	endformat

;;; file manager format (works!)
	jsr	$3DC		; load up Y and A
	sty	fmptr
	sta	fmptr+1

	lda	#$0B		; init command
	ldy	#0
	sta	(fmptr),y

	lda	#$9D		; DOS location
	ldy	#1
	sta	(fmptr),y

	lda	#254		; volume number
	ldy	#4
	sta	(fmptr),y

	lda	#$01		; drive number
	ldy	#5
	sta	(fmptr),y

	lda	#$06		; slot number
	ldy	#6
	sta	(fmptr),y

	lda	#$00		; scratch area LSB
	ldy	#$0C
	sta	(fmptr),y

	lda	#$92		; scratch area MSB
	ldy	#$0D
	sta	(fmptr),y

	jsr	$3D6		; doit!

	ldy	#$0A		; return code
	lda	(fmptr),y
	beq	endformat
formaterror:
	jmp	diskerror
endformat:

;;;begin segment loop (5)
	lda	#0		; 256 bytes/sector
	ldy	#$0b		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#0		; buffer LSB
	ldy	#8		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#0
	sta	trknum		; start with track 0
	lda	#5
	sta	segcnt
segloop:

;;; fancy status here
;	jsr	status
;	lda	#<waitm		; print waiting for data
;	ldy	#>waitm
;	jsr	print
;countdown:
;	lda	#<count		; store begin location LSB
;	sta	begload
;	lda	#>count		; store begin location MSB
;	sta	begload+1
;	lda	#<count+4	; store end location LSB
;	sta	endload
;	lda	#>count		; store end location MSB
;	sta	endload+1
;;;; hack readtape, fix later, POC for now
;	lda	#$60		; return without check
;	sta	$9091
;	jsr	readtape	; get the code
;	lda	#$8A		; put TXA back
;	sta	$9091
;;;; end hack
;	lda	#18
;	sta	$24		; horiz
;	lda	#22		; vert
;	jsr	movecur		; move cursor to $24,a; 0 base
;	jsr	cleos
;	lda	#<count		; print count down
;	ldy	#>count
;	jsr	print
;	lda	count
;	cmp	#$B0
;	bne	countdown
;	lda	count+1
;	cmp	#$B0
;	bne	countdown
;;; end fancy stuff

;;; get 7 tracks from tape
load:
	jsr	status
	lda	#<loadm		; print loading data
	ldy	#>loadm
	jsr	flash
	lda	#<loadm2	; print loading data
	ldy	#>loadm2
	jsr	print

	sec
	lda	#5
	sbc	segcnt
	asl
	asl
	tax
	stx	temp

	lda	infdata+2,x	; get sec
	jsr	cout
	lda	infdata+3,x	; get sec
	beq	second
	jsr	cout
second:
	lda	#<secm		; print sec
	ldy	#>secm
	jsr	print

	ldx	temp
	lda	infdata+0,x	; store begin location LSB
	sta	begload
	lda	infdata+1,x	; store begin location MSB
	sta	begload+1

	lda	#$00		; store end location LSB
	sta	endload
	lda	#$90		; store end location MSB
	sta	endload+1

	jsr	readtape	; get the code
inf:
				; turn motor on to save 1-2 sec
	ldx	#$60		; slot #6
	lda	motoron,x	; turn it on

	jsr	status
	lda	#<inflatem	; print inflating
	ldy	#>inflatem
	jsr	print

	ldx	temp
	lda	infdata+0,x	;src lsb
	sta	inf_zp+0
	lda	infdata+1,x	;src msb
	sta	inf_zp+1
	lda	#<data		;dst lsb
	sta	inf_zp+2
	lda	#>data		;dst msb
	sta	inf_zp+3

	jsr	inflate

	lda	#$00		;dst end +1 lsb
	cmp	inf_zp+2
	bne	error
	lda	#$80		;dst end +1 msb
	cmp	inf_zp+3
	bne	error

;;;begin track loop (7)
	jsr	status
	lda	#<writem	; print writing
	ldy	#>writem
	jsr	print

	lda	#>data
	sta	buffer
	lda	#7
	sta	trkcnt		; do 7 tracks/segment
trkloop:
	lda	trknum		; track number
	ldy	#4		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

;	lda	#0		; seek(0) command
;	ldy	#$0C		; offset in RWTS
;	sta	(pointer),y	; write it to RWTS

;	jsr	locrpl		; locate rwts paramlist
;	jsr	rwts		; do it!
;	bcs	diskerror

;;;begin sector loop (16), backwards is faster, much faster
	lda	#$F
	sta	secnum
secloop:
	;jsr	draw_w		; write sector from buffer to disk
	jsr	draw_s		; write sector from buffer to disk
	lda	secnum		; sector number
	ldy	#5		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	buffer		; buffer MSB
	clc
	adc	secnum
	ldy	#9		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	lda	#2		; read(1)/write(2) command
	ldy	#$0C		; offset in RWTS
	sta	(pointer),y	; write it to RWTS

	jsr	locrpl		; locate rwts paramlist
	jsr	rwts		; do it!
	bcs	diskerror
	lda	#0
	sta	preg		; fix p reg so mon is happy

	;jsr	draw_r		; read sector from disk to compare addr
	;lda	#>cmpbuf	; compare MSB
	;ldy	#9		; offset in RWTS
	;sta	(pointer),y	; write it to RWTS

	;lda	#1		; read(1)/write(2) command
	;ldy	#$0C		; offset in RWTS
	;sta	(pointer),y	; write it to RWTS

	;jsr	locrpl		; locate rwts paramlist
	;jsr	rwts		; do it!
	;bcs	diskerror
	;lda	#0
	;sta	preg		; fix p reg so mon is happy

	;;; compare code

	;jsr	draw_s		; draw a space in the grid if OK

	dec	secnum
	bpl	secloop
;;;end sector loop

	lda	buffer		; buffer += $10
	clc
	adc	#$10
	sta	buffer

	inc	trknum		; next track
	dec	trkcnt		;
	bne	trkloop		; 0, all done with 7 tracks
;;;end track loop

	dec	segcnt		;
	beq	done		; 0, all done with 5 segments
	jmp	segloop
;;;end segment loop

;;; prompt for data only load?
done:
	jsr	status
	lda	#<donem		; print done
	ldy	#>donem
	jsr	print
	jsr	bell
	jsr	rdkey
	jmp	reboot
error:
				; turn motor off, just in case left on
	ldx	#$60		; slot #6
	lda	motoroff,x	; turn it off

	lda	#<errorm	; print error
	ldy	#>errorm
	jsr	print
	jmp	warm
diskerror:
	lda	#0
	sta	preg		; fix p reg so mon is happy
	jsr	status
	lda	#<diskerrorm	; print error
	ldy	#>diskerrorm
	jsr	print
	jmp	warm
status:
	lda	#0
	sta	$24		; horiz
	lda	#22		; vert
	jsr	movecur		; move cursor to $24,a; 0 base
	jmp	cleos
draw_w:				; print a 'W' in the grid
	clc
	lda	#4
	adc	secnum
	tay
	lda	#4
	adc	trknum
	ldx	#'W'
	jmp	draw
draw_r:				; print a 'R' in the grid
	clc
	lda	#4
	adc	secnum
	tay
	lda	#4
	adc	trknum
	ldx	#'R'
	jmp	draw
draw_s:				; print a ' ' in the grid
	clc
	lda	#4
	adc	secnum
	tay
	lda	#4
	adc	trknum
	ldx	#invsp
draw:				; a=horiz, y=vert, x=letter
	sta	$24		; horiz
	tya
	jsr	movecur
	txa
	eor	#$40
	jsr	cout
	rts
line:
	lda	#'-'
	ora	#$80
loop0:
	jsr	cout
	dex
	bne	loop0
	jsr	crout
	rts
inv:
	sta	prtptr
	sty	prtptr+1
	ldy	#0
	lda	(prtptr),y	; load initial char
inv1:	and	#$3F
	jsr	cout
	iny
	lda	(prtptr),y
	bne	inv1
	rts
flash:
	sta	prtptr
	sty	prtptr+1
	ldy	#0
	lda	(prtptr),y	; load initial char
flash1:	ora	#$40
	jsr	cout
	iny
	lda	(prtptr),y
	bne	flash1
	rts
print:
	sta	prtptr
	sty	prtptr+1
	ldy	#0
	lda	(prtptr),y	; load initial char
print1: ora	#$80
	jsr	cout
	iny
	lda	(prtptr),y
	bne	print1
	rts
title:
	.asciiz	"INSTA-DISK"
errorm:
	.asciiz	"ERROR"
diskerrorm:
	.asciiz	"DISK ERROR"
donem:
	.asciiz	"DONE. PRESS [RETURN] TO REBOOT."
inflatem:
	.asciiz	"INFLATING DATA "
loadm:
	.asciiz	"LOADING DATA"
loadm2:
	.asciiz	", ETA "
secm:
	.asciiz	" SEC. "
formatm:
	.asciiz	"FORMATTING DISK "
waitm:
	.asciiz	"WAITING FOR DATA: "
writem:
	.asciiz	"WRITING DATA "
track:
	.byte	"TRACK",$0D,0
header:
	.byte   "              1111111111222222222233333",$0D
	.byte   "    01234567890123456789012345678901234",$0D
	.byte	"    ",0
left:
	.byte	"  0|",$0D
	.byte	"  1|",$0D
	.byte	"  2|",$0D
	.byte	"  3|",$0D
	.byte	"  4|",$0D
	.byte	"S 5|",$0D
	.byte	"E 6|",$0D
	.byte	"C 7|",$0D
	.byte	"T 8|",$0D
	.byte	"O 9|",$0D
	.byte	"R A|",$0D
	.byte	"  B|",$0D
	.byte	"  C|",$0D
	.byte	"  D|",$0D
	.byte	"  E|",$0D
	.byte	"  F|",$0D,0
infdata:
	;.byte	0,0,0,0		; LSB/MSB start, ETA in sec
	;.byte	0,0,0,0		; LSB/MSB start, ETA in sec
	;.byte	0,0,0,0		; LSB/MSB start, ETA in sec
	;.byte	0,0,0,0		; LSB/MSB start, ETA in sec
	;.byte	0,0,0,0		; LSB/MSB start, ETA in sec
	;.byte	0		; format flag, 1 = no format
