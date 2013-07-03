package main

import (
	"fmt"
	"proto"
)

func main() {
	p := proto.NewTest()

	p.Setf1(1)
	p.Setf2(2)
	p.Setf3(3)
	p.Setf4(4)
	p.Setf5(5)
	p.Setf6(6)
	p.Setf7(7)
	p.Setf8(8)
	p.Setf9(9)
	p.Setf10(10)
	p.Setf11(11)
	p.Setf12(12)
	p.Setf13(true)
	p.Setf14("hello 中文")
	p.Setf15([]byte{0, 0})
	p.Setf16(proto.Test_Type_Type2)

	b := proto.NewSub()
	b.Setf1(11)
	b.Setf2(12)
	b.SetF3(13)
	p.Setf17(b)

	for i := 0; i < 5; i++ {
		p.Addf18(int32(i))
	}
	p.Addf19(proto.Test_Type_Type3)
	p.Addf19(proto.Test_Type_Type1)

	for i := 0; i < 6; i++ {
		sub := proto.NewSub()
		p.Addf20(sub)
		sub.Setf1(int32(i*10 + 1))
		sub.Setf2(int32(i*10 + 2))
		sub.SetF3(int32(i*10 + 3))
	}

	for i := 0; i < 3; i++ {
		p.Addf21([]byte{0, 0, 0})
	}
	for i := 0; i < 5; i++ {
		p.Addf22(int32(i))
	}

	buf := proto.NewProtoBuffer(1000)
	p.Serialize(buf)
	buf.PrintBuffer()

	size := p.ByteSize()
	newt := proto.NewTest()
	for i := 0; i < 10; i++ {
		e := newt.Parse(buf, size)
		if e != nil {
			fmt.Println(e)
		}
	}
}
