; J. Kent Wirant
; 30 Nov. 2022
; ECE 1895 - Project 3
; Bootloader

section .text
	global start_boot ; name of entry point
	
bits 16
;org 0x7C00

; ======== MASTER BOOT RECORD (MBR) ===========================================
; The first 512 bytes of the boot drive are loaded into memory and executed.
; This code/data (the MBR) must read and execute additional code from a disk 
; drive. This is called the "second stage" of the bootloader. The BIOS 
; supports basic disk I/O operations and video display functions, which this
; source code utilizes for loading the second stage and for status reporting.
; =============================================================================

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
	
	; load second stage into memory
	mov ebx, 1 		; start from second sector (first is index 0)
	mov cx, 15		; read 15 sectors
	mov dl, 0x80	; hard disk drive 0 (boot drive)
	mov si, 0x7E00	; destination address
	call read_disk
	
	; print message and begin stage 2
	mov si, str_stage2
	call print_str
	jmp stage2
	
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
	
; -----------------------------------------------------------------------------
; Reads a specified number of 512-byte disk sectors into memory.
; EBX - LBA of first sector to read
; CX - number of sectors to read (maximum might be 0x79)
; DL - drive number (e.g. 1st HDD is 0x80)
; DS:SI - address to write to
; https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=42h:_Extended_Read_Sectors_From_Drive
read_disk:
	pushf
	pusha
	
	; populate disk address packet
	mov [disk_address_packet_len], cx
	lea eax, [ds:si]
	mov [disk_address_packet_mem], eax
	mov [disk_address_packet_lba], ebx
	
	; load interrupt arguments
	xor ax, ax
	mov ds, ax
	mov si, disk_address_packet
	mov ah, 0x42 	; Function number for Extended Read Sectors from Drive
	int 0x13		; call BIOS to read disk sectors into memory
	
	; if no error, exit function, otherwise print error message and hang
	jnc .success
	mov si, str_err_read
	call print_str
	cli
	hlt
	
.success:
	popa
	popf
	ret
	
	
; DATA AND VARIABLES ----------------------------------------------------------

str_boot:					db 'Booting OS...', 13, 10, 0
str_err_read:				db 'Disk read error!', 13, 10, 0

disk_address_packet: 		db 0x10, 0		; size of DAP followed by 0
disk_address_packet_len:	dw 0			; number of sectors to read
disk_address_packet_mem:	dd 0			; destination address of read
disk_address_packet_lba:	dq 0			; logical block address

; BOOT SIGNATURE --------------------------------------------------------------

times (510 - $ + $$) db 00 ; padding (signature is present in last two bytes)
dw 0xAA55 ; signature of bootable drive

; ======== SECOND STAGE =======================================================
; This part of the program is present on the hard disk after the first sector.
; The MBR calls upon the BIOS to load this part into memory. The second stage
; of the bootloader must set up the execution environment for the operating
; system, then transition from 16-bit Real Mode into 32-bit Protected Mode.
; Setting up the execution environment may include using the BIOS to create
; a map of the address space, collecting information regarding the PCI bus,
; and others, but much of this is outside the scope of this project. 
; Transitioning from Real Mode to Protected Mode requires setting up a 
; Global Descriptor Table (GDT) to describe the 32-bit memory space. This
; implementation uses the Flat Memory Model, where the whole address space
; is readable, writable, and executable from the kernel and user space.
; =============================================================================

stage2:
	cli
	lgdt [gdt_descriptor]	; load global descriptor table
	mov eax, cr0
	or al, 1 				; set PE (protection enable) bit to 1
	mov cr0, eax
	jmp (gdt_kernel_code - gdt_null):proc_pmode_start ; far jump to 32-bit code
	hlt

; DATA AND VARIABLES ----------------------------------------------------------
str_stage2:					db 'Second stage loaded.', 13, 10, 0

; GLOBAL DESCRIPTOR TABLE -----------------------------------------------------
; https://wiki.osdev.org/Global_Descriptor_Table

align 8 ; gdt must be on 8-byte boundary (for descriptor may be unnecessary)
gdt_descriptor:
	dw (gdt_end - gdt_null - 1) ; size
	dd gdt_null ; offset
	
align 8 ; gdt must be on 8-byte boundary
gdt_null:			dq 0
gdt_kernel_code:	dq 0x00CF9A000000FFFF ; 4 GiB range, ring 0, readable code
gdt_kernel_data:	dq 0x00CF92000000FFFF ; 4 GiB range, ring 0, writable data
gdt_user_code:		dq 0x00CFFA000000FFFF ; 4 GiB range, ring 3, readable code
gdt_user_data:		dq 0x00CFF2000000FFFF ; 4 GiB range, ring 3, writable data
gdt_end:

; ======== PROTECTED MODE =====================================================
; This part of the bootloader runs in 32-bit Protected Mode. It hands over 
; control to the cross-compiled C code. Developing the operating system in C
; is more productive and less error-prone than developing in assembly language.
; =============================================================================

[bits 32]
[extern _start]

; -----------------------------------------------------------------------------
proc_pmode_start:
	mov ax, (gdt_kernel_data - gdt_null) ; load segment registers
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov esp, 0x7C00 ; stack just below boot sector
	
	; print message by writing to video memory (white text, black background)
	; odd byte addresses are color format, even byte addresses are ASCII
	mov ebx, 0xB8000 ; video memory address
	mov dword [ebx+320], 0x0F6E0F45 ; "En" (starting on third row (2*2*80))
	mov dword [ebx+324], 0x0F650F74 ; "te"
	mov dword [ebx+328], 0x0F650F72 ; "re"
	mov dword [ebx+332], 0x0F200F64 ; "d "
	mov dword [ebx+336], 0x0F720F50 ; "Pr"
	mov dword [ebx+340], 0x0F740F6F ; "ot"
	mov dword [ebx+344], 0x0F630F65 ; "ec"
	mov dword [ebx+348], 0x0F650F74 ; "te"
	mov dword [ebx+352], 0x0F200F64 ; "d "
	mov dword [ebx+356], 0x0F6F0F4D ; "Mo"
	mov dword [ebx+360], 0x0F650F64 ; "de"
	mov dword [ebx+364], 0x0F200F2E ; ". " ("Entered Protected Mode. ")
	
	cli
	jmp _start ; begin C code
	hlt
	
; -----------------------------------------------------------------------------
	