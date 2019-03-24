    mov ax,cs
    mov ss,ax
    mov sp,0x7c00

    mov ah,0x06
    mov al,0   ;清窗口

    mov ch,5   ;左上角的行号
    mov cl,5   ;左上角的列号
    mov dh,20  ;右下角的行号
    mov dl,74  ;右下角的行号
    mov bh,0x17;属性为蓝底白字
    int 0x10

  @1: 
    jmp @1

    times 510-($-$$) db 0
                     db 0x55,0xaa
;以下是注释说明:
;功能号：06H/07H 
;  用　途:窗口内容向上/向下滚动 
;  参　数:AL＝要滚动的行数,若是0将清窗口
;  BH＝填入新行的属性 
;  CH＝滚动窗口的左上角行号 
;  CL＝滚动窗口的左上角列号
;  DH＝滚动窗口的右下角行号
;  DL＝滚动窗口的右下角列号 
;  调　用:INT 10H 
;  返　回:无

