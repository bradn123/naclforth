	.file	"test1.c"
	.section	.rodata
	.align 4
	.type	kNaClSrpcProtocolVersion, @object
	.size	kNaClSrpcProtocolVersion, 4
kNaClSrpcProtocolVersion:
	.long	-1059454974
	.align 4
	.type	kSrpcSocketAddressDescriptorNumber, @object
	.size	kSrpcSocketAddressDescriptorNumber, 4
kSrpcSocketAddressDescriptorNumber:
	.long	4
	.align 4
	.type	kInvalidDescriptorNumber, @object
	.size	kInvalidDescriptorNumber, 4
kInvalidDescriptorNumber:
	.long	-1
	.text
	.align 32
.globl BoolMethod
	.type	BoolMethod, @function
BoolMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	12(%ebp), %edx
	movl	(%edx), %edx
	movl	8(%edx), %edx
	testl	%edx, %edx
	sete	%dl
	movzbl	%dl, %edx
	movl	%edx, 8(%eax)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	BoolMethod, .-BoolMethod
	.align 32
.globl DoubleMethod
	.type	DoubleMethod, @function
DoubleMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	12(%ebp), %edx
	movl	(%edx), %edx
	fldl	8(%edx)
	fchs
	fstpl	8(%eax)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	DoubleMethod, .-DoubleMethod
	.align 32
.globl NaNMethod
	.type	NaNMethod, @function
NaNMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$40, %esp
	movl	$1074266111, -16(%ebp)
	movl	$-1, -12(%ebp)
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	addl	$8, %eax
	movl	%eax, %ecx
	movl	-16(%ebp), %eax
	movl	-12(%ebp), %edx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	NaNMethod, .-NaNMethod
	.align 32
.globl IntMethod
	.type	IntMethod, @function
IntMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	12(%ebp), %edx
	movl	(%edx), %edx
	movl	8(%edx), %edx
	negl	%edx
	movl	%edx, 8(%eax)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	IntMethod, .-IntMethod
	.align 32
.globl LongMethod
	.type	LongMethod, @function
LongMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %ecx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	12(%eax), %edx
	movl	8(%eax), %eax
	negl	%eax
	adcl	$0, %edx
	negl	%edx
	movl	%eax, 8(%ecx)
	movl	%edx, 12(%ecx)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	LongMethod, .-LongMethod
	.align 32
.globl StringMethod
	.type	StringMethod, @function
StringMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$20, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %ebx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	%eax, (%esp)
	call	strlen
	movl	%eax, 8(%ebx)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$20, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	StringMethod, .-StringMethod
	.section	.rodata
	.align 4
.LC1:
	.string	"CharArrayMethod: count mismatch: in=%d out=%d\n"
	.text
	.align 32
.globl CharArrayMethod
	.type	CharArrayMethod, @function
CharArrayMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$36, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	cmpl	%eax, %edx
	je	.L14
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%eax, %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%edx, 8(%esp)
	movl	%eax, 4(%esp)
	movl	$.LC1, (%esp)
	call	printf
	movl	8(%ebp), %eax
	movl	$268, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	.align 32
.L14:
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	$0, -16(%ebp)
	jmp	.L15
	.align 32
.L16:
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %edx
	movl	-12(%ebp), %ecx
	movl	%ecx, %ebx
	subl	%edx, %ebx
	movl	%ebx, %edx
	subl	$1, %edx
	leal	(%eax,%edx), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %ecx
	movl	-16(%ebp), %eax
	leal	(%ecx,%eax), %eax
	movzbl	(%eax), %eax
	movb	%al, (%edx)
	addl	$1, -16(%ebp)
	.align 32
.L15:
	movl	-16(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jl	.L16
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$36, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	CharArrayMethod, .-CharArrayMethod
	.align 32
.globl DoubleArrayMethod
	.type	DoubleArrayMethod, @function
DoubleArrayMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$36, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	cmpl	%eax, %edx
	je	.L19
	movl	8(%ebp), %eax
	movl	$268, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	.align 32
.L19:
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	$0, -16(%ebp)
	jmp	.L20
	.align 32
.L21:
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %edx
	movl	-12(%ebp), %ecx
	movl	%ecx, %ebx
	subl	%edx, %ebx
	movl	%ebx, %edx
	subl	$1, %edx
	sall	$3, %edx
	leal	(%eax,%edx), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %ecx
	sall	$3, %ecx
	addl	%ecx, %eax
	fldl	(%eax)
	fstpl	(%edx)
	addl	$1, -16(%ebp)
	.align 32
.L20:
	movl	-16(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jl	.L21
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$36, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	DoubleArrayMethod, .-DoubleArrayMethod
	.align 32
.globl IntArrayMethod
	.type	IntArrayMethod, @function
IntArrayMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$36, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	cmpl	%eax, %edx
	je	.L24
	movl	8(%ebp), %eax
	movl	$268, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	.align 32
.L24:
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	$0, -16(%ebp)
	jmp	.L25
	.align 32
.L26:
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %edx
	movl	-12(%ebp), %ecx
	movl	%ecx, %ebx
	subl	%edx, %ebx
	movl	%ebx, %edx
	subl	$1, %edx
	sall	$2, %edx
	leal	(%eax,%edx), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %ecx
	sall	$2, %ecx
	addl	%ecx, %eax
	movl	(%eax), %eax
	movl	%eax, (%edx)
	addl	$1, -16(%ebp)
	.align 32
.L25:
	movl	-16(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jl	.L26
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$36, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	IntArrayMethod, .-IntArrayMethod
	.align 32
.globl LongArrayMethod
	.type	LongArrayMethod, @function
LongArrayMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$36, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %edx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	cmpl	%eax, %edx
	je	.L29
	movl	8(%ebp), %eax
	movl	$268, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	.align 32
.L29:
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	$0, -16(%ebp)
	jmp	.L30
	.align 32
.L31:
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %edx
	movl	-12(%ebp), %ecx
	movl	%ecx, %ebx
	subl	%edx, %ebx
	movl	%ebx, %edx
	subl	$1, %edx
	sall	$3, %edx
	leal	(%eax,%edx), %ecx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	16(%eax), %eax
	movl	-16(%ebp), %edx
	sall	$3, %edx
	addl	%edx, %eax
	movl	4(%eax), %edx
	movl	(%eax), %eax
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	addl	$1, -16(%ebp)
	.align 32
.L30:
	movl	-16(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jl	.L31
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$36, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	LongArrayMethod, .-LongArrayMethod
	.align 32
.globl NullMethod
	.type	NullMethod, @function
NullMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	NullMethod, .-NullMethod
	.align 32
.globl ReturnStringMethod
	.type	ReturnStringMethod, @function
ReturnStringMethod:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$20, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %ebx
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %eax
	addl	$string.2773, %eax
	movl	%eax, (%esp)
	call	strdup
	movl	%eax, 16(%ebx)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	addl	$20, %esp
	popl	%ebx
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	ReturnStringMethod, .-ReturnStringMethod
	.align 32
.globl HandleMethod
	.type	HandleMethod, @function
HandleMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	HandleMethod, .-HandleMethod
	.align 32
.globl ReturnHandleMethod
	.type	ReturnHandleMethod, @function
ReturnHandleMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	kSrpcSocketAddressDescriptorNumber, %edx
	movl	%edx, 8(%eax)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	ReturnHandleMethod, .-ReturnHandleMethod
	.align 32
.globl InvalidHandleMethod
	.type	InvalidHandleMethod, @function
InvalidHandleMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	12(%ebp), %eax
	movl	(%eax), %eax
	movl	8(%eax), %edx
	movl	kInvalidDescriptorNumber, %eax
	cmpl	%eax, %edx
	jne	.L42
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	jmp	.L43
	.align 32
.L42:
	movl	8(%ebp), %eax
	movl	$268, 16(%eax)
	.align 32
.L43:
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	InvalidHandleMethod, .-InvalidHandleMethod
	.align 32
.globl ReturnInvalidHandleMethod
	.type	ReturnInvalidHandleMethod, @function
ReturnInvalidHandleMethod:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	16(%ebp), %eax
	movl	(%eax), %eax
	movl	kInvalidDescriptorNumber, %edx
	movl	%edx, 8(%eax)
	movl	8(%ebp), %eax
	movl	$256, 16(%eax)
	movl	20(%ebp), %eax
	movl	(%eax), %eax
	movl	20(%ebp), %edx
	movl	%edx, (%esp)
	naclcall	%eax
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	ReturnInvalidHandleMethod, .-ReturnInvalidHandleMethod
.globl srpc_methods
	.section	.rodata
.LC2:
	.string	"bool:b:b"
.LC3:
	.string	"double:d:d"
.LC4:
	.string	"nan::d"
.LC5:
	.string	"int:i:i"
.LC6:
	.string	"long:l:l"
.LC7:
	.string	"string:s:i"
.LC8:
	.string	"char_array:C:C"
.LC9:
	.string	"double_array:D:D"
.LC10:
	.string	"int_array:I:I"
.LC11:
	.string	"long_array:L:L"
.LC12:
	.string	"null_method::"
.LC13:
	.string	"stringret:i:s"
.LC14:
	.string	"handle:h:"
.LC15:
	.string	"handleret::h"
.LC16:
	.string	"invalid_handle:h:"
.LC17:
	.string	"invalid_handle_ret::h"
	.align 32
	.type	srpc_methods, @object
	.size	srpc_methods, 136
srpc_methods:
	.long	.LC2
	.long	BoolMethod
	.long	.LC3
	.long	DoubleMethod
	.long	.LC4
	.long	NaNMethod
	.long	.LC5
	.long	IntMethod
	.long	.LC6
	.long	LongMethod
	.long	.LC7
	.long	StringMethod
	.long	.LC8
	.long	CharArrayMethod
	.long	.LC9
	.long	DoubleArrayMethod
	.long	.LC10
	.long	IntArrayMethod
	.long	.LC11
	.long	LongArrayMethod
	.long	.LC12
	.long	NullMethod
	.long	.LC13
	.long	ReturnStringMethod
	.long	.LC14
	.long	HandleMethod
	.long	.LC15
	.long	ReturnHandleMethod
	.long	.LC16
	.long	InvalidHandleMethod
	.long	.LC17
	.long	ReturnInvalidHandleMethod
	.long	0
	.long	0
.LC18:
	.string	"_etext: %x\n"
.LC19:
	.string	"_edata: %x\n"
.LC20:
	.string	"_end: %x\n"
.LC21:
	.string	"brk: %x\n"
	.text
	.align 32
.globl main
	.type	main, @function
main:
	pushl	%ebp
	movl	%esp, %ebp
	andl	$-16, %esp
	subl	$16, %esp
	movl	__etext__, %eax
	movl	%eax, 4(%esp)
	movl	$.LC18, (%esp)
	call	printf
	movl	_edata, %eax
	movl	%eax, 4(%esp)
	movl	$.LC19, (%esp)
	call	printf
	movl	_end, %eax
	movl	%eax, 4(%esp)
	movl	$.LC20, (%esp)
	call	printf
	movl	$0, (%esp)
	call	sysbrk
	movl	%eax, 4(%esp)
	movl	$.LC21, (%esp)
	call	printf
	call	NaClSrpcModuleInit
	testl	%eax, %eax
	jne	.L48
	movl	$1, %eax
	jmp	.L49
	.align 32
.L48:
	movl	$srpc_methods, (%esp)
	call	NaClSrpcAcceptClientConnection
	testl	%eax, %eax
	jne	.L50
	movl	$1, %eax
	jmp	.L49
	.align 32
.L50:
	call	NaClSrpcModuleFini
	movl	$0, %eax
	.align 32
.L49:
	movl	%ebp, %esp
	popl	%ebp
	popl	%ecx
	nacljmp	%ecx
	.size	main, .-main
	.data
	.align 32
	.type	string.2773, @object
	.size	string.2773, 450
string.2773:
	.ascii	"Ich weiss nicht, was soll es bedeutenDass ich so traurig bin"
	.ascii	",Ein Maerchen aus uralten Zeiten,Das kommt mir nicht aus dem"
	.ascii	" Sinn.Die Luft ist kuehl und es dunkelt,Und ruhig fliesst de"
	.ascii	"r Rhein;Der G"
	.string	"ipfel des Berges funkelt,Im Abendsonnenschein.Die schoenste Jungfrau sitzetDort oben wunderbar,Ihr gold'nes Geschmeide blitzet,Sie kaemmt ihr goldenes Haar,Sie kaemmt es mit goldenem Kamme,Und singt ein Lied dabei;Das hat eine wundersame,Gewalt'ge Melodei."
	.ident	"GCC: (GNU) 4.4.3 20101221 (Native Client r4027)"
	.section	.note.GNU-stack,"",@progbits
