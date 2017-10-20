; Copyright 2017 Google Inc.
;
; Use of this source code is governed by a BSD-style
; license that can be found in the LICENSE file or at
; https://developers.google.com/open-source/licenses/bsd

; Sense / binary loading code for 23kbps (average) waveform.

TEXT_BASE = $80
Y_IX = $82
X_IX = $83
TEMP = $84
TOGGLE = $85 ; 0 for first nibble, 1 for second

    .code
    .org $bf00

main:
    lda $C050
    lda $C052
    lda $C057
    lda #0
    sta TOGGLE
    sta TEXT_BASE
    ldx #$20
    stx TEXT_BASE + 1
start_sync:
    ldx #0
start_sync0:
    inx
    bit $c060
    bmi start_sync0
    cpx #$8
    rol
    ;sta (TEXT_BASE),y ;(for visualizing)
    ;iny
start_sync1:
    bit $c060
    bpl start_sync1
    ; want 42 from this edge to cycle
    cmp #$55 ; sense leader
    bne start_sync
    lda #0
    nop
    nop
    ; sleep 21
    ldy #4
sleep0:
    dey
    bne sleep0

cycle0:
    ; sleep 11
    pha
    pla
    nop
    nop
cycle:
    ldx #$ff
sync0:
    inx        ;2
    bit $c060  ;4
    bpl sync0a ;2
    bit $c060  ;4
    bpl sync0a ;2
    bit $c060  ;4
    bpl sync0a ;2
    bit $c060  ;4
    bmi sync0  ;3 += 27
sync0a:
    asl      ;2
    asl      ;2
    stx TEMP ;3
    adc TEMP ;3
    ldx #$ff
sync1:
    inx
    bit $c060
    bmi sync1a
    bit $c060
    bmi sync1a
    bit $c060
    bmi sync1a
    bit $c060
    bpl sync1
sync1a:
    ; each path from here to cycle is 42 cycles
    asl      ;2
    asl      ;2
    stx TEMP ;3
    adc TEMP ;3

    dec TOGGLE ;6
    bne nibble ;2
    ; we have byte
    sta (TEXT_BASE), y ;6
    lda #0             ;2
    iny                ;2
    ; 28
    bne cycle0      ;2
    inc TEXT_BASE+1 ;5
    nop             ;2
    nop             ;2
    jmp cycle       ;3
    ; 42 (sbould be correct)
nibble:
    ; 19 cycles from sync1a
    ldx #1          ;2
    stx TOGGLE      ;3
    ldx TEXT_BASE+1 ;3
    cpx #$40        ;2
    beq done        ;3
    pha     ;(sleep) 3
    pla             ;4
    jmp cycle       ;3


done:
    jmp main
