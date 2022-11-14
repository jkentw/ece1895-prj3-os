; J. Kent Wirant
; 14 Nov. 2022
; ECE 1895 - Project 3
; Bootloader

section .text
	global start_boot ; name of entry point
	
bits 16
;org 0x7C00

; ======== MASTER BOOT RECORD (MBR) ==========================================
; The first 512 bytes of the boot drive are loaded into memory and executed.
; This code/data (the MBR) must read and execute additional code from a disk 
; drive. This is called the "second stage" of the bootloader. The BIOS 
; supports basic disk I/O operations and video display functions, which this
; source code utilizes for loading the second stage and for status reporting.
; ============================================================================

; entry point of bootloader
start_boot:
	; setup
	cli 				; disable interrupts
	mov sp, 0x7A00		; set up stack (grows downward)
	xor ax, ax			; set segment registers (except cs) to 0 
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; print boot message
	mov si, str_boot
	call print_str
	
	; halt the program (best practice)
	cli
	hlt
	
; -----------------------------------------------------------------------------
; prints null-terminated string using BIOS.
; DS:SI - pointer to null-terminated string
; DF - direction flag is used for printing in reverse direction
print_str:
	pushf				; save registers
	push ax
	push bx
	push si
	mov ah, 0x0E		; teletype output mode
	xor bx, bx
	jmp .load
.loop:					; while char at DS:SI != 0, print char and continue 
	int 0x10
.load:
	lodsb
	test al, al
	jnz .loop
.end:
	pop si				; load the registers we saved earlier
	pop bx
	pop ax
	popf
	ret
	
; DATA AND VARIABLES ----------------------------------------------------------

str_boot: db 'Booting OS...', 13, 10, 0

; BOOT SIGNATURE --------------------------------------------------------------

times (510 - $ + $$) db 00 ; padding (signature is present in last two bytes)
dw 0xAA55 ; signature of bootable drive
	