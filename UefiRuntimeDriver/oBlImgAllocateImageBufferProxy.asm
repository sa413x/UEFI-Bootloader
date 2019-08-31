EXTERN gBlImgAllocateImageBuffer: qword
PUBLIC oBlImgAllocateImageBufferProxy

.code

oBlImgAllocateImageBufferProxy PROC
mov QWORD PTR [rsp + 10h], rbx
jmp [gBlImgAllocateImageBuffer]
oBlImgAllocateImageBufferProxy ENDP

END