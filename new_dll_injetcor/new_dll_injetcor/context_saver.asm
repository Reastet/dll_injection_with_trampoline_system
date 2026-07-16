.code 

message proto

context proc
    push RBX
    push RCX
    push RDX
    push RSI
    push RDI
    push R8
    push R9
    push R10
    push R11
    push R12
    push R13
    push R14
    push R15
    call message
    pop R15
    pop R14
    pop R13
    pop R12
    pop R11
    pop R10
    pop R9
    pop R8
    pop RDI
    pop RSI
    pop RDX
    pop RCX
    pop RBX
    jmp rax
    ret
context endp
end
