	.def __asm_fletcher16

	.ref Modulus255

	.thumb
	.align 2
	.text

__asm_fletcher16:
	.asmfunc

	PUSH {R3 - R7}	;	not using r7-r11
	PUSH {LR}

	MOV R7, R0	;	R0 contains first parameter (pointer to data)
	MOV R4, R1	;	R1 contains second parameter (count)
	MOV R11, R2	; 	R2 contains third parameter (dim)


	eor R5, R5, R5	;	sum1=0
	eor R6, R6, R6	; 	sum2=0
	eor R8, R8, R8	;	index=0
	eor R9, R9, R9	; 	index_outer = 0
forloop2:
	ADD R9, R9, #1	; ++index_outer
	eor R8, R8, R8	; index = 0 (reset inner loop)
forloop1:
	ADD R8, R8, #1 		;	++index
	LDRSB R10, [R7], #1	;	*data++
	ADD R5, R5, R10 	;	sum1 = sum1 +data
	MOV R0, R5			; Modulus255(sum1)
	BL Modulus255		; call
	MOV R5, R0			; sum1 = ret

	ADD R6, R6, R5		;	sum2 = sum2 + sum1
	MOV R0, R6			; Modulus255(sum2)
	BL Modulus255		; call
	MOV R6, R0			; sum2 = ret

	; loop condition
	CMP R8, R4		;	index - count < 0?
	BLT forloop1

	;	outer loop condition
	CMP R9, R11		; outer_index - dim  0;?
	BLT forloop2

	; done
	; bit shift then return
	LSL R6, R6, #8	;	sum2 << 8
	ORR	R0, R6, R5	;	sum2 | sum1

	;/**
	    ;uint16_t sum1 = 0;
	    ;uint16_t sum2 = 0;
	    ;int index;

	   ; for( index = 0; index < count; ++index )
	   ; {
	   ;     sum1 = (sum1 + data[index]) % 255;
	   ;     sum2 = (sum2 + sum1) % 255;
	   ; }

	   ; return (sum2 << 8) | sum1;



		;uint16_t Modulus255(uint16_t input)
		;{
		;    return (input % 255);
		;}


	;;**/
forloop2_end:
	POP {LR} ; restore registers
	POP {R3 - R7}
	BX LR ; branch back using LR (goes with POP LR)

	.endasmfunc

	.align
	.end
