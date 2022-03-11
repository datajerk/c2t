; inflate - uncompress data stored in the DEFLATE format
; by Piotr Fusik <fox@scene.pl>
; Last modified: 2007-06-17

; Compile with xasm (http://xasm.atari.org/), for example:
; xasm inflate.asx /l /d:inflate=$b700 /d:inflate_data=$b900 /d:inflate_zp=$f0
; inflate is 509 bytes of code and initialized data
; inflate_data is 764 bytes of uninitialized data
; inflate_zp is 10 bytes on page zero

; hacked for apple ii and ca65 by Egan Ford <egan@sense.net>
; dates

; compile nodes

; changes

;EFF
.define	equ	=

inflate		=	$BA00
inflate_zp	=	$0
inflate_data	=	$BC00

; Pointer to compressed data
inputPointer                    equ	inflate_zp    ; 2 bytes

; Pointer to uncompressed data
outputPointer                   equ	inflate_zp+2  ; 2 bytes

; Local variables

getBit_buffer                   equ	inflate_zp+4  ; 1 byte

getBits_base                    equ	inflate_zp+5  ; 1 byte
inflateStoredBlock_pageCounter  equ	inflate_zp+5  ; 1 byte

inflateCodes_sourcePointer      equ	inflate_zp+6  ; 2 bytes
inflateDynamicBlock_lengthIndex equ	inflate_zp+6  ; 1 byte
inflateDynamicBlock_lastLength	equ	inflate_zp+7  ; 1 byte
inflateDynamicBlock_tempCodes   equ	inflate_zp+7  ; 1 byte

inflateCodes_lengthMinus2       equ	inflate_zp+8  ; 1 byte
inflateDynamicBlock_allCodes    equ	inflate_zp+8  ; 1 byte

inflateCodes_primaryCodes       equ	inflate_zp+9  ; 1 byte


; Argument values for getBits
GET_1_BIT                       equ	$81
GET_2_BITS                      equ	$82
GET_3_BITS                      equ	$84
GET_4_BITS                      equ	$88
GET_5_BITS                      equ	$90
GET_6_BITS                      equ	$a0
GET_7_BITS                      equ	$c0

; Maximum length of a Huffman code
MAX_CODE_LENGTH                 equ	15

; Huffman trees
TREE_SIZE                       equ	MAX_CODE_LENGTH+1
PRIMARY_TREE                    equ	0
DISTANCE_TREE                   equ	TREE_SIZE

; Alphabet
LENGTH_SYMBOLS                  equ	1+29+2
DISTANCE_SYMBOLS                equ	30
CONTROL_SYMBOLS                 equ	LENGTH_SYMBOLS+DISTANCE_SYMBOLS
TOTAL_SYMBOLS                   equ	256+CONTROL_SYMBOLS

; Optional (recommend for c2t or DOS)
; DOS header location and size LSB/MSB
;	.byte   <START,>START,<(END-START),>(END-START)

; Uncompress DEFLATE stream starting from the address stored in inputPointer
; to the memory starting from the address stored in outputPointer
;;	org	inflate
	.org	inflate
START:
;;	mvy	#0	getBit_buffer
	LDY	#0
	STY	getBit_buffer
	
	
inflate_blockLoop:
; Get a bit of EOF and two bits of block type
;	ldy	#0
	sty	getBits_base
	lda	#GET_3_BITS
	jsr	getBits
;;	lsr	@
	lsr	A
	php
	tax
	bne	inflateCompressedBlock

; Copy uncompressed block
;	ldy	#0
	sty	getBit_buffer
	jsr	getWord
	jsr	getWord
	sta	inflateStoredBlock_pageCounter
;	jmp	inflateStoredBlock_firstByte
	bcs	inflateStoredBlock_firstByte
inflateStoredBlock_copyByte:
	jsr	getByte
inflateStoreByte:
	jsr	storeByte
	bcc	inflateCodes_loop
inflateStoredBlock_firstByte:
	inx
	bne	inflateStoredBlock_copyByte
	inc	inflateStoredBlock_pageCounter
	bne	inflateStoredBlock_copyByte

inflate_nextBlock:
	plp
	bcc	inflate_blockLoop
	rts

inflateCompressedBlock:

; Decompress a block with fixed Huffman trees
; :144 dta 8
; :112 dta 9
; :24  dta 7
; :6   dta 8
; :2   dta 8 ; codes with no meaning
; :30  dta 5+DISTANCE_TREE
;	ldy	#0
inflateFixedBlock_setCodeLengths:
	lda	#4
	cpy	#144
;;	rol	@
	rol	A
	sta	literalSymbolCodeLength,y
	cpy	#CONTROL_SYMBOLS
	bcs	inflateFixedBlock_noControlSymbol
	lda	#5+DISTANCE_TREE
	cpy	#LENGTH_SYMBOLS
	bcs	inflateFixedBlock_setControlCodeLength
	cpy	#24
	adc	#(2-DISTANCE_TREE) & $ff
inflateFixedBlock_setControlCodeLength:
	sta	controlSymbolCodeLength,y
inflateFixedBlock_noControlSymbol:
	iny
	bne	inflateFixedBlock_setCodeLengths
;	mva	#LENGTH_SYMBOLS	inflateCodes_primaryCodes
	LDA	#LENGTH_SYMBOLS
	STA	inflateCodes_primaryCodes

	dex
	beq	inflateCodes

; Decompress a block reading Huffman trees first

; Build the tree for temporary codes
	jsr	buildTempHuffmanTree

; Use temporary codes to get lengths of literal/length and distance codes
	ldx	#0
;	sec
inflateDynamicBlock_decodeLength:
	php
	stx	inflateDynamicBlock_lengthIndex
; Fetch a temporary code
	jsr	fetchPrimaryCode
; Temporary code 0..15: put this length
	tax
	bpl	inflateDynamicBlock_verbatimLength
; Temporary code 16: repeat last length 3 + getBits(2) times
; Temporary code 17: put zero length 3 + getBits(3) times
; Temporary code 18: put zero length 11 + getBits(7) times
	jsr	getBits
;	sec
	adc	#1
	cpx	#GET_7_BITS
;;	scc:adc	#7
	BCC	S1
	adc	#7
S1:
	tay
	lda	#0
	cpx	#GET_3_BITS
;;	scs:lda	inflateDynamicBlock_lastLength
	BCS	S2
	lda	inflateDynamicBlock_lastLength
S2:
inflateDynamicBlock_verbatimLength:
	iny
	ldx	inflateDynamicBlock_lengthIndex
	plp
inflateDynamicBlock_storeLength:
	bcc	inflateDynamicBlock_controlSymbolCodeLength
;;	sta	literalSymbolCodeLength,x+
	sta	literalSymbolCodeLength,x
	INX
	cpx	#1
inflateDynamicBlock_storeNext:
	dey
	bne	inflateDynamicBlock_storeLength
	sta	inflateDynamicBlock_lastLength
;	jmp	inflateDynamicBlock_decodeLength
	beq	inflateDynamicBlock_decodeLength
inflateDynamicBlock_controlSymbolCodeLength:
	cpx	inflateCodes_primaryCodes
;;	scc:ora	#DISTANCE_TREE
	BCC	S3
	ora	#DISTANCE_TREE
S3:
;;	sta	controlSymbolCodeLength,x+
	sta	controlSymbolCodeLength,x
	INX	

	cpx	inflateDynamicBlock_allCodes
	bcc	inflateDynamicBlock_storeNext
	dey
;	ldy	#0
;	jmp	inflateCodes

; Decompress a block
inflateCodes:
	jsr	buildHuffmanTree
inflateCodes_loop:
	jsr	fetchPrimaryCode
	bcc	inflateStoreByte
	tax
	beq	inflate_nextBlock
; Copy sequence from look-behind buffer
;	ldy	#0
	sty	getBits_base
	cmp	#9
	bcc	inflateCodes_setSequenceLength
	tya
;	lda	#0
	cpx	#1+28
	bcs	inflateCodes_setSequenceLength
	dex
	txa
;;	lsr	@
	lsr	A
	ror	getBits_base
	inc	getBits_base
;;	lsr	@
	lsr	A
	rol	getBits_base
	jsr	getAMinus1BitsMax8
;	sec
	adc	#0
inflateCodes_setSequenceLength:
	sta	inflateCodes_lengthMinus2
	ldx	#DISTANCE_TREE
	jsr	fetchCode
;	sec
	sbc	inflateCodes_primaryCodes
	tax
	cmp	#4
	bcc	inflateCodes_setOffsetLowByte
	inc	getBits_base
;;	lsr	@
	lsr	A
	jsr	getAMinus1BitsMax8
inflateCodes_setOffsetLowByte:
	eor	#$ff
	sta	inflateCodes_sourcePointer
	lda	getBits_base
	cpx	#10
	bcc	inflateCodes_setOffsetHighByte
	lda	getNPlus1Bits_mask-10,x
	jsr	getBits
	clc
inflateCodes_setOffsetHighByte:
	eor	#$ff
;	clc
	adc	outputPointer+1
	sta	inflateCodes_sourcePointer+1
	jsr	copyByte
	jsr	copyByte
inflateCodes_copyByte:
	jsr	copyByte
	dec	inflateCodes_lengthMinus2
	bne	inflateCodes_copyByte
;	jmp	inflateCodes_loop
	beq	inflateCodes_loop

buildTempHuffmanTree:
;	ldy	#0
	tya
inflateDynamicBlock_clearCodeLengths:
	sta	literalSymbolCodeLength,y
	sta	literalSymbolCodeLength+TOTAL_SYMBOLS-256,y
	iny
	bne	inflateDynamicBlock_clearCodeLengths
; numberOfPrimaryCodes = 257 + getBits(5)
; numberOfDistanceCodes = 1 + getBits(5)
; numberOfTemporaryCodes = 4 + getBits(4)
	ldx	#3
inflateDynamicBlock_getHeader:
	lda	inflateDynamicBlock_headerBits-1,x
	jsr	getBits
;	sec
	adc	inflateDynamicBlock_headerBase-1,x
	sta	inflateDynamicBlock_tempCodes-1,x
	sta	inflateDynamicBlock_headerBase+1
	dex
	bne	inflateDynamicBlock_getHeader

; Get lengths of temporary codes in the order stored in tempCodeLengthOrder
;	ldx	#0
inflateDynamicBlock_getTempCodeLengths:
	lda	#GET_3_BITS
	jsr	getBits
	ldy	tempCodeLengthOrder,x
	sta	literalSymbolCodeLength,y
	ldy	#0
	inx
	cpx	inflateDynamicBlock_tempCodes
	bcc	inflateDynamicBlock_getTempCodeLengths

; Build Huffman trees basing on code lengths (in bits)
; stored in the *SymbolCodeLength arrays
buildHuffmanTree:
; Clear nBitCode_totalCount, nBitCode_literalCount, nBitCode_controlCount
	tya
;	lda	#0
;;	sta:rne	nBitCode_clearFrom,y+
R1:
	sta	nBitCode_clearFrom,y
	INY
	BNE	R1
; Count number of codes of each length
;	ldy	#0
buildHuffmanTree_countCodeLengths:
	ldx	literalSymbolCodeLength,y
	inc	nBitCode_literalCount,x
	inc	nBitCode_totalCount,x
	cpy	#CONTROL_SYMBOLS
	bcs	buildHuffmanTree_noControlSymbol
	ldx	controlSymbolCodeLength,y
	inc	nBitCode_controlCount,x
	inc	nBitCode_totalCount,x
buildHuffmanTree_noControlSymbol:
	iny
	bne	buildHuffmanTree_countCodeLengths
; Calculate offsets of symbols sorted by code length
;	lda	#0
	ldx	#(-3*TREE_SIZE) & $ff
buildHuffmanTree_calculateOffsets:
	sta	nBitCode_literalOffset+3*TREE_SIZE-$100,x
;;	add	nBitCode_literalCount+3*TREE_SIZE-$100,x
	CLC
	ADC	nBitCode_literalCount+3*TREE_SIZE-$100,x
	inx
	bne	buildHuffmanTree_calculateOffsets
; Put symbols in their place in the sorted array
;	ldy	#0
buildHuffmanTree_assignCode:
	tya
	ldx	literalSymbolCodeLength,y
;;	ldy:inc	nBitCode_literalOffset,x
	ldy	nBitCode_literalOffset,x
	inc	nBitCode_literalOffset,x
	sta	codeToLiteralSymbol,y
	tay
	cpy	#CONTROL_SYMBOLS
	bcs	buildHuffmanTree_noControlSymbol2
	ldx	controlSymbolCodeLength,y
;;	ldy:inc	nBitCode_controlOffset,x
	ldy	nBitCode_controlOffset,x
	inc	nBitCode_controlOffset,x
	sta	codeToControlSymbol,y
	tay
buildHuffmanTree_noControlSymbol2:
	iny
	bne	buildHuffmanTree_assignCode
	rts

; Read Huffman code using the primary tree
fetchPrimaryCode:
	ldx	#PRIMARY_TREE
; Read a code from input basing on the tree specified in X,
; return low byte of this code in A,
; return C flag reset for literal code, set for length code
fetchCode:
;	ldy	#0
	tya
fetchCode_nextBit:
	jsr	getBit
;;	rol	@
	rol	A
	inx
;;	sub	nBitCode_totalCount,x
	SEC
	SBC	nBitCode_totalCount,x
	bcs	fetchCode_nextBit
;	clc
	adc	nBitCode_controlCount,x
	bcs	fetchCode_control
;	clc
	adc	nBitCode_literalOffset,x
	tax
	lda	codeToLiteralSymbol,x
	clc
	rts
fetchCode_control:
;;	add	nBitCode_controlOffset-1,x
	CLC
	ADC	nBitCode_controlOffset-1,x
	tax
	lda	codeToControlSymbol,x
	sec
	rts

; Read A minus 1 bits, but no more than 8
getAMinus1BitsMax8:
	rol	getBits_base
	tax
	cmp	#9
	bcs	getByte
	lda	getNPlus1Bits_mask-2,x
getBits:
	jsr	getBits_loop
getBits_normalizeLoop:
	lsr	getBits_base
;;	ror	@
	ror	A
	bcc	getBits_normalizeLoop
	rts

; Read 16 bits
getWord:
	jsr	getByte
	tax
; Read 8 bits
getByte:
	lda	#$80
getBits_loop:
	jsr	getBit
;;	ror	@
	ror	A
	bcc	getBits_loop
	rts

; Read one bit, return in the C flag
getBit:
	lsr	getBit_buffer
	bne	getBit_return
	pha
;	ldy	#0
	lda	(inputPointer),y
;;	inw	inputPointer
	INC	inputPointer
	BNE	S4
	INC	inputPointer+1
S4:
	sec
;;	ror	@
	ror	A
	sta	getBit_buffer
	pla
getBit_return:
	rts

; Copy a previously written byte
copyByte:
	ldy	outputPointer
	lda	(inflateCodes_sourcePointer),y
	ldy	#0
; Write a byte
storeByte:
	sta	(outputPointer),y
	inc	outputPointer
	bne	storeByte_return
	inc	outputPointer+1
	inc	inflateCodes_sourcePointer+1
storeByte_return:
	rts

getNPlus1Bits_mask:
	.byte	GET_1_BIT,GET_2_BITS,GET_3_BITS,GET_4_BITS,GET_5_BITS,GET_6_BITS,GET_7_BITS

tempCodeLengthOrder:
	.byte	GET_2_BITS,GET_3_BITS,GET_7_BITS,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15

inflateDynamicBlock_headerBits:
	.byte	GET_4_BITS,GET_5_BITS,GET_5_BITS
inflateDynamicBlock_headerBase:
	.byte	3,0,0  ; second byte is modified at runtime!

	.org inflate_data

; Data for building trees

literalSymbolCodeLength:
	.org	*+256
controlSymbolCodeLength:
	.org	*+CONTROL_SYMBOLS

; Huffman trees

nBitCode_clearFrom:
nBitCode_totalCount:
	.org	*+2*TREE_SIZE
nBitCode_literalCount:
	.org	*+TREE_SIZE
nBitCode_controlCount:
	.org	*+2*TREE_SIZE
nBitCode_literalOffset:
	.org	*+TREE_SIZE
nBitCode_controlOffset:
	.org	*+2*TREE_SIZE

codeToLiteralSymbol:
	.org	*+256
codeToControlSymbol:
	.org	*+CONTROL_SYMBOLS

END:
