#ifndef _CODE_H_
#define _CODE_H_

#define GO_CODE "import ( \n\
	\"fmt\"\n\
	\"unsafe\"\n\
)\n\
\n\
const (\n\
	SIZE_32BIT       = 4\n\
	SIZE_64BIT       = 8\n\
	WIRE_TYPE_VARINT = 0\n\
	WIRE_TYPE_64BIT  = 1\n\
	WIRE_TYPE_LENGTH = 2\n\
	WIRE_TYPE_32_BIT = 5\n\
)\n\
\n\
func Encode32(buf []byte, index int, number uint32) int {\n\
	if number < 0x80 {\n\
		buf[index+0] = byte(number)\n\
		return 1\n\
	}\n\
	buf[index+0] = byte(number | 0x80)\n\
	if number < 0x4000 {\n\
		buf[index+1] = byte(number >> 7)\n\
		return 2\n\
	}\n\
	buf[index+1] = byte((number >> 7) | 0x80)\n\
	if number < 0x200000 {\n\
		buf[index+2] = byte(number >> 14)\n\
		return 3\n\
	}\n\
	buf[index+2] = byte((number >> 14) | 0x80)\n\
	if number < 0x10000000 {\n\
		buf[index+3] = byte(number >> 21)\n\
		return 4\n\
	}\n\
	buf[index+3] = byte((number >> 21) | 0x80)\n\
	buf[index+4] = byte(number >> 28)\n\
	return 5\n\
}\n\
\n\
func Encode32Size(number uint32) int {\n\
	switch {\n\
	case number < 0x80:\n\
		return 1\n\
	case number < 0x4000:\n\
		return 2\n\
	case number < 0x200000:\n\
		return 3\n\
	case number < 0x10000000:\n\
		return 4\n\
	default:\n\
		return 5\n\
	}\n\
}\n\
\n\
func Encode64(buf []byte, index int, number uint64) int {\n\
	if (number & 0xffffffff) == number {\n\
		return Encode32(buf, index, uint32(number))\n\
	}\n\
\n\
	i := 0\n\
	for ; number >= 0x80; i++ {\n\
		buf[index+i] = byte(number | 0x80)\n\
		number >>= 7\n\
	}\n\
	buf[index+i] = byte(number)\n\
	return i + 1\n\
}\n\
\n\
func Encode64Size(number uint64) int {\n\
	if (number & 0xffffffff) == number {\n\
		return Encode32Size(uint32(number))\n\
	}\n\
\n\
	i := 0\n\
	for ; number >= 0x80; i++ {\n\
		number >>= 7\n\
	}\n\
	return i + 1\n\
}\n\
\n\
func Decode(buf []byte, index int) (x uint64, n int) {\n\
	buf = buf[index:]\n\
\n\
	for shift := uint(0); ; shift += 7 {\n\
		if n >= len(buf) {\n\
			return 0, n\n\
		}\n\
\n\
		b := uint64(buf[n])\n\
		n++\n\
		x |= (b & 0x7F) << shift\n\
		if (b & 0x80) == 0 {\n\
			break\n\
		}\n\
	}\n\
	return x, n\n\
}\n\
\n\
func Zigzag32(n int32) int32 {\n\
	return (n << 1) ^ (n >> 31)\n\
}\n\
\n\
func Zigzag64(n int64) int64 {\n\
	return (n << 1) ^ (n >> 63)\n\
}\n\
\n\
func Dezigzag32(n int32) int32 {\n\
	return (int32(n) >> 1) ^ -(n & 1)\n\
}\n\
\n\
func Dezigzag64(n int64) int64 {\n\
	return (int64(n) >> 1) ^ -(n & 1)\n\
}\n\
\n\
func StringToBytes(s string) []byte {\n\
	l := len(s)\n\
	ret := make([]byte, l)\n\
	copy(ret, s)\n\
	return ret\n\
}\n\
\n\
func BooleanToInt(b bool) int {\n\
	if b {\n\
		return 1\n\
	} else {\n\
		return 0\n\
	}\n\
}\n\
\n\
func WriteInt32Size(x int32) int {\n\
	if x < 0 {\n\
		return Encode64Size(uint64(x))\n\
	} else {\n\
		return Encode32Size(uint32(x))\n\
	}\n\
}\n\
\n\
func WriteInt64Size(x int64) int {\n\
	return Encode64Size(uint64(x))\n\
}\n\
\n\
func WriteUInt32Size(x uint32) int {\n\
	return Encode32Size(x)\n\
}\n\
\n\
func WriteUInt64Size(x uint64) int {\n\
	return Encode64Size(x)\n\
}\n\
\n\
func WriteSInt32Size(x int32) int {\n\
	v := Zigzag32(x)\n\
	return Encode32Size(uint32(v))\n\
}\n\
\n\
func WriteSInt64Size(x int64) int {\n\
	v := Zigzag64(x)\n\
	return Encode64Size(uint64(v))\n\
}\n\
\n\
func WriteStringSize(s string) int {\n\
	return len(s)\n\
}\n\
\n\
//ProtoBuffer\n\
type ProtoBuffer struct {\n\
	buf []byte\n\
	pos int\n\
}\n\
\n\
func NewProtoBuffer(buf []byte) *ProtoBuffer {\n\
	buffer := new(ProtoBuffer)\n\
	buffer.buf = buf\n\
\n\
	return buffer\n\
}\n\
\n\
func (p *ProtoBuffer) AddPos(v int) {\n\
	if v <= 0 {\n\
		return\n\
	}\n\
\n\
	p.pos += v\n\
	if p.pos > len(p.buf) {\n\
		p.pos = len(p.buf)\n\
	}\n\
}\n\
\n\
func (p *ProtoBuffer) ResetPos() {\n\
	p.pos = 0\n\
}\n\
\n\
func (p *ProtoBuffer) PrintBuffer() {\n\
	fmt.Println(\"Size:\", p.pos)\n\
	for i := 0; i < p.pos; i++ {\n\
		fmt.Printf(\"%2.2X \", p.buf[i])\n\
	}\n\
	fmt.Println()\n\
}\n\
\n\
//int32\n\
func (p *ProtoBuffer) WriteInt32(x int32) {\n\
	if x < 0 {\n\
		p.AddPos(Encode64(p.buf, p.pos, uint64(x)))\n\
	} else {\n\
		p.AddPos(Encode32(p.buf, p.pos, uint32(x)))\n\
	}\n\
}\n\
\n\
func (p *ProtoBuffer) ReadInt32() int32 {\n\
	x, n := Decode(p.buf, p.pos)\n\
	p.AddPos(n)\n\
	return int32(x)\n\
}\n\
\n\
//uint32\n\
func (p *ProtoBuffer) WriteUInt32(x uint32) {\n\
	p.AddPos(Encode32(p.buf, p.pos, uint32(x)))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadUInt32() uint32 {\n\
	x, n := Decode(p.buf, p.pos)\n\
	p.AddPos(n)\n\
	return uint32(x)\n\
}\n\
\n\
//int64\n\
func (p *ProtoBuffer) WriteInt64(x int64) {\n\
	p.AddPos(Encode64(p.buf, p.pos, uint64(x)))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadInt64() int64 {\n\
	x, n := Decode(p.buf, p.pos)\n\
	p.AddPos(n)\n\
	return int64(x)\n\
}\n\
\n\
//uint64\n\
func (p *ProtoBuffer) WriteUInt64(x uint64) {\n\
	p.AddPos(Encode64(p.buf, p.pos, uint64(x)))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadUInt64() uint64 {\n\
	x, n := Decode(p.buf, p.pos)\n\
	p.AddPos(n)\n\
	return x\n\
}\n\
\n\
//sint32\n\
func (p *ProtoBuffer) WriteSInt32(x int32) {\n\
	v := Zigzag32(x)\n\
	p.WriteInt32(v)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadSInt32() int32 {\n\
	v := p.ReadInt32()\n\
	return Dezigzag32(v)\n\
}\n\
\n\
//sint64\n\
func (p *ProtoBuffer) WriteSInt64(x int64) {\n\
	v := Zigzag64(x)\n\
	p.WriteInt64(v)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadSInt64() int64 {\n\
	v := p.ReadInt64()\n\
	return Dezigzag64(v)\n\
}\n\
\n\
//sfixed32\n\
func (p *ProtoBuffer) WriteSFixed32(x int32) {\n\
	p.WriteFixed32(uint32(x))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadSFixed32() int32 {\n\
	return int32(p.ReadFixed32())\n\
}\n\
\n\
//sfixed64\n\
func (p *ProtoBuffer) WriteSFixed64(x int64) {\n\
	p.WriteFixed64(uint64(x))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadSFixed64() int64 {\n\
	return int64(p.ReadFixed64())\n\
}\n\
\n\
//fixed32\n\
func (p *ProtoBuffer) WriteFixed32(x uint32) {\n\
	buf := p.buf\n\
	pos := p.pos\n\
\n\
	buf[pos] = uint8(x)\n\
	buf[pos+1] = uint8(x >> 8)\n\
	buf[pos+2] = uint8(x >> 16)\n\
	buf[pos+3] = uint8(x >> 24)\n\
	p.AddPos(SIZE_32BIT)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadFixed32() uint32 {\n\
	buf := p.buf\n\
	pos := p.pos\n\
	p.AddPos(SIZE_32BIT)\n\
\n\
	if p.pos >= len(buf) {\n\
		return 0\n\
	}\n\
\n\
	x := uint32(buf[pos]) |\n\
		(uint32(buf[pos+1]) << 8) |\n\
		(uint32(buf[pos+2]) << 16) |\n\
		(uint32(buf[pos+3]) << 24)\n\
	return x\n\
}\n\
\n\
//fixed64\n\
func (p *ProtoBuffer) WriteFixed64(x uint64) {\n\
	buf := p.buf\n\
	pos := p.pos\n\
	p.AddPos(SIZE_64BIT)\n\
\n\
	buf[pos] = uint8(x)\n\
	buf[pos+1] = uint8(x >> 8)\n\
	buf[pos+2] = uint8(x >> 16)\n\
	buf[pos+3] = uint8(x >> 24)\n\
	buf[pos+4] = uint8(x >> 32)\n\
	buf[pos+5] = uint8(x >> 40)\n\
	buf[pos+6] = uint8(x >> 48)\n\
	buf[pos+7] = uint8(x >> 56)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadFixed64() uint64 {\n\
	buf := p.buf\n\
	pos := p.pos\n\
	p.AddPos(SIZE_64BIT)\n\
\n\
	if p.pos >= len(buf) {\n\
		return 0\n\
	}\n\
\n\
	ret_low := uint32(buf[pos]) |\n\
		(uint32(buf[pos+1]) << 8) |\n\
		(uint32(buf[pos+2]) << 16) |\n\
		(uint32(buf[pos+3]) << 24)\n\
	ret_high := uint32(buf[pos+4]) |\n\
		(uint32(buf[pos+5]) << 8) |\n\
		(uint32(buf[pos+6]) << 16) |\n\
		(uint32(buf[pos+7]) << 24)\n\
	return (uint64(ret_high) << 32) | uint64(ret_low)\n\
}\n\
\n\
//float32\n\
func (p *ProtoBuffer) WriteFloat32(f float32) {\n\
	p.WriteFixed32(*(*uint32)(unsafe.Pointer(&f)))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadFloat32() float32 {\n\
	x := p.ReadFixed32()\n\
	return *(*float32)(unsafe.Pointer(&x))\n\
}\n\
\n\
//float64\n\
func (p *ProtoBuffer) WriteFloat64(f float64) {\n\
	p.WriteFixed64(*(*uint64)(unsafe.Pointer(&f)))\n\
}\n\
\n\
func (p *ProtoBuffer) ReadFloat64() float64 {\n\
	x := p.ReadFixed64()\n\
	return *(*float64)(unsafe.Pointer(&x))\n\
}\n\
\n\
//boolean\n\
func (p *ProtoBuffer) WriteBoolean(b bool) {\n\
	if b {\n\
		p.WriteInt32(1)\n\
	} else {\n\
		p.WriteInt32(0)\n\
	}\n\
}\n\
\n\
func (p *ProtoBuffer) ReadBoolean() bool {\n\
	x := p.ReadInt32()\n\
	return x == 1\n\
}\n\
\n\
//string\n\
func (p *ProtoBuffer) WriteString(s string) {\n\
	l := len(s)\n\
	p.WriteUInt32(uint32(l))\n\
	copy(p.buf[p.pos:], s)\n\
	p.AddPos(l)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadString() string {\n\
	l := p.ReadUInt32()\n\
	old_pos := p.pos\n\
	p.AddPos(int(l))\n\
	s := string(p.buf[old_pos:p.pos])\n\
	return s\n\
}\n\
\n\
//[]byte\n\
func (p *ProtoBuffer) WriteBytes(b []byte) {\n\
	l := len(b)\n\
	p.WriteUInt32(uint32(l))\n\
	copy(p.buf[p.pos:], b)\n\
	p.AddPos(l)\n\
}\n\
\n\
func (p *ProtoBuffer) ReadBytes() []byte {\n\
	l := p.ReadUInt32()\n\
	old_pos := p.pos\n\
	p.AddPos(int(l))\n\
\n\
	b := make([]byte, l)\n\
	copy(b, p.buf[old_pos:p.pos])\n\
	return b\n\
}\n\
\n\
func (p *ProtoBuffer) GetUnknowFieldValueSize(wire_tag int32) {\n\
	wire_type := wire_tag & 0x7\n\
\n\
	switch wire_type {\n\
	case WIRE_TYPE_VARINT:\n\
		p.ReadUInt32()\n\
	case WIRE_TYPE_64BIT:\n\
		p.ReadFixed64()\n\
	case WIRE_TYPE_LENGTH:\n\
		size := p.ReadInt32()\n\
		if size > 0 {\n\
			p.AddPos(int(size))\n\
		}\n\
	case WIRE_TYPE_32_BIT:\n\
		p.ReadFixed32()\n\
	}\n\
}\n\
\n\
//error\n\
type ProtoError struct {\n\
	What string\n\
}\n\
\n\
func (e *ProtoError) Error() string {\n\
	return e.What\n\
}\n\
\n\
//proto interface\n\
type Message interface {\n\
	Serialize(buf []byte) (size int, err error)\n\
	Parse(buf []byte, msg_size int) error\n\
	Clear()\n\
	ByteSize() int\n\
	IsInitialized() bool\n\
}\n "


#endif//_CODE_H_
