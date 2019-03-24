    mov ax,cs
    mov ss,ax
    mov sp,0x7c00

    mov ah,0x06
    mov al,0

    mov ch,5  ;(5,5)
    mov cl,5
    mov dh,5  ;(5,74)
    mov dl,74
    mov bh,0x17 ;蓝底白字
    int 0x10

    mov ch,6  ;(6,5)
    mov cl,5
    mov dh,6  ;(6,74)
    mov dl,74
    mov bh,0x27 ;绿底白字
    int 0x10

    mov ch,7  ;(7,5)
    mov cl,5
    mov dh,7  ;(7,74)
    mov dl,74
    mov bh,0x37 ;青底白字
    int 0x10    

    mov ch,8  ;(8,5)
    mov cl,5
    mov dh,8  ;(8,74)
    mov dl,74
    mov bh,0x47 ;红底白字
    int 0x10

@1:
    mov ah,0x00
    int 0x16

    mov ah,0x06
    mov al,1

    mov ch,5
    mov cl,5
    mov dh,8
    mov dl,74
    mov bh,0x77
    int 0x10

    jmp @1

    times 510-($-$$) db 0
                     db 0x55,0xaa

;以下是注释说明:
;当ah的内容是0x00时,执行int 0x16后,中断服务例程会监视键盘动作,当它返回时,会在寄存器al中存放按键的ASCII码 
;本实验我们不关心按下哪个键,只关心按键这个动作,当中断返回时,说明用户按键了,这时候我们让窗口上滚一行,这样4行就变成3行了
;最底下这行我们用白色填充,然后再次调用键盘中断,当用户按键后继续上滚,依然用白色填充最下面一行
;当按键4次后窗口被完全滚出看到的是4行白色