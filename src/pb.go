import ( 
	"fmt"
	"unsafe"
)

const (
	SIZE_32BIT       = 4
	SIZE_64BIT       = 8
	WIRE_TYPE_VARINT = 0
	WIRE_TYPE_64BIT  = 1
	WIRE_TYPE_LENGTH = 2
	WIRE_TYPE_32_BIT = 5
)

func Encode32(buf []byte, index int, number uint32) int {
	if number < 0x80 {
		buf[index+0] = byte(number)
		return 1
	}
	buf[index+0] = byte(number | 0x80)
	if number < 0x4000 {
		buf[index+1] = byte(number >> 7)
		return 2
	}
	buf[index+1] = byte((number >> 7) | 0x80)
	if number < 0x200000 {
		buf[index+2] = byte(number >> 14)
		return 3
	}
	buf[index+2] = byte((number >> 14) | 0x80)
	if number < 0x10000000 {
		buf[index+3] = byte(number >> 21)
		return 4
	}
	buf[index+3] = byte((number >> 21) | 0x80)
	buf[index+4] = byte(number >> 28)
	return 5
}

func Encode32Size(number uint32) int {
	switch {
	case number < 0x80:
		return 1
	case number < 0x4000:
		return 2
	case number < 0x200000:
		return 3
	case number < 0x10000000:
		return 4
	default:
		return 5
	}
}

func Encode64(buf []byte, index int, number uint64) int {
	if (number & 0xffffffff) == number {
		return Encode32(buf, index, uint32(number))
	}

	i := 0
	for ; number >= 0x80; i++ {
		buf[index+i] = byte(number | 0x80)
		number >>= 7
	}
	buf[index+i] = byte(number)
	return i + 1
}

func Encode64Size(number uint64) int {
	if (number & 0xffffffff) == number {
		return Encode32Size(uint32(number))
	}

	i := 0
	for ; number >= 0x80; i++ {
		number >>= 7
	}
	return i + 1
}

func Decode(buf []byte, index int) (x uint64, n int) {
	buf = buf[index:]

	for shift := uint(0); ; shift += 7 {
		if n >= len(buf) {
			return 0, n
		}

		b := uint64(buf[n])
		n++
		x |= (b & 0x7F) << shift
		if (b & 0x80) == 0 {
			break
		}
	}
	return x, n
}

func Zigzag32(n int32) int32 {
	return (n << 1) ^ (n >> 31)
}

func Zigzag64(n int64) int64 {
	return (n << 1) ^ (n >> 63)
}

func Dezigzag32(n int32) int32 {
	return (int32(n) >> 1) ^ -(n & 1)
}

func Dezigzag64(n int64) int64 {
	return (int64(n) >> 1) ^ -(n & 1)
}

func StringToBytes(s string) []byte {
	l := len(s)
	ret := make([]byte, l)
	copy(ret, s)
	return ret
}

func BooleanToInt(b bool) int {
	if b {
		return 1
	} else {
		return 0
	}
}

func WriteInt32Size(x int32) int {
	if x < 0 {
		return Encode64Size(uint64(x))
	} else {
		return Encode32Size(uint32(x))
	}
}

func WriteInt64Size(x int64) int {
	return Encode64Size(uint64(x))
}

func WriteUInt32Size(x uint32) int {
	return Encode32Size(x)
}

func WriteUInt64Size(x uint64) int {
	return Encode64Size(x)
}

func WriteSInt32Size(x int32) int {
	v := Zigzag32(x)
	return Encode32Size(uint32(v))
}

func WriteSInt64Size(x int64) int {
	v := Zigzag64(x)
	return Encode64Size(uint64(v))
}

func WriteStringSize(s string) int {
	return len(s)
}

//ProtoBuffer
type ProtoBuffer struct {
	buf []byte
	pos int
}

func NewProtoBuffer(len int) *ProtoBuffer {
	buffer := new(ProtoBuffer)
	buffer.buf = make([]byte, len)

	return buffer
}

func (p *ProtoBuffer) AddPos(v int) {
	if v <= 0 {
		return
	}

	p.pos += v
	if p.pos > len(p.buf) {
		p.pos = len(p.buf)
	}
}

func (p *ProtoBuffer) ResetPos() {
	p.pos = 0
}

func (p *ProtoBuffer) PrintBuffer() {
	fmt.Println("Size:", p.pos)
	for i := 0; i < p.pos; i++ {
		fmt.Printf("%2.2X ", p.buf[i])
	}
	fmt.Println()
}

//int32
func (p *ProtoBuffer) WriteInt32(x int32) {
	if x < 0 {
		p.AddPos(Encode64(p.buf, p.pos, uint64(x)))
	} else {
		p.AddPos(Encode32(p.buf, p.pos, uint32(x)))
	}
}

func (p *ProtoBuffer) ReadInt32() int32 {
	x, n := Decode(p.buf, p.pos)
	p.AddPos(n)
	return int32(x)
}

//uint32
func (p *ProtoBuffer) WriteUInt32(x uint32) {
	p.AddPos(Encode32(p.buf, p.pos, uint32(x)))
}

func (p *ProtoBuffer) ReadUInt32() uint32 {
	x, n := Decode(p.buf, p.pos)
	p.AddPos(n)
	return uint32(x)
}

//int64
func (p *ProtoBuffer) WriteInt64(x int64) {
	p.AddPos(Encode64(p.buf, p.pos, uint64(x)))
}

func (p *ProtoBuffer) ReadInt64() int64 {
	x, n := Decode(p.buf, p.pos)
	p.AddPos(n)
	return int64(x)
}

//uint64
func (p *ProtoBuffer) WriteUInt64(x uint64) {
	p.AddPos(Encode64(p.buf, p.pos, uint64(x)))
}

func (p *ProtoBuffer) ReadUInt64() uint64 {
	x, n := Decode(p.buf, p.pos)
	p.AddPos(n)
	return x
}

//sint32
func (p *ProtoBuffer) WriteSInt32(x int32) {
	v := Zigzag32(x)
	p.WriteInt32(v)
}

func (p *ProtoBuffer) ReadSInt32() int32 {
	v := p.ReadInt32()
	return Dezigzag32(v)
}

//sint64
func (p *ProtoBuffer) WriteSInt64(x int64) {
	v := Zigzag64(x)
	p.WriteInt64(v)
}

func (p *ProtoBuffer) ReadSInt64() int64 {
	v := p.ReadInt64()
	return Dezigzag64(v)
}

//sfixed32
func (p *ProtoBuffer) WriteSFixed32(x int32) {
	p.WriteFixed32(uint32(x))
}

func (p *ProtoBuffer) ReadSFixed32() int32 {
	return int32(p.ReadFixed32())
}

//sfixed64
func (p *ProtoBuffer) WriteSFixed64(x int64) {
	p.WriteFixed64(uint64(x))
}

func (p *ProtoBuffer) ReadSFixed64() int64 {
	return int64(p.ReadFixed64())
}

//fixed32
func (p *ProtoBuffer) WriteFixed32(x uint32) {
	buf := p.buf
	pos := p.pos

	buf[pos] = uint8(x)
	buf[pos+1] = uint8(x >> 8)
	buf[pos+2] = uint8(x >> 16)
	buf[pos+3] = uint8(x >> 24)
	p.AddPos(SIZE_32BIT)
}

func (p *ProtoBuffer) ReadFixed32() uint32 {
	buf := p.buf
	pos := p.pos
	p.AddPos(SIZE_32BIT)

	if p.pos >= len(buf) {
		return 0
	}

	x := uint32(buf[pos]) |
		(uint32(buf[pos+1]) << 8) |
		(uint32(buf[pos+2]) << 16) |
		(uint32(buf[pos+3]) << 24)
	return x
}

//fixed64
func (p *ProtoBuffer) WriteFixed64(x uint64) {
	buf := p.buf
	pos := p.pos
	p.AddPos(SIZE_64BIT)

	buf[pos] = uint8(x)
	buf[pos+1] = uint8(x >> 8)
	buf[pos+2] = uint8(x >> 16)
	buf[pos+3] = uint8(x >> 24)
	buf[pos+4] = uint8(x >> 32)
	buf[pos+5] = uint8(x >> 40)
	buf[pos+6] = uint8(x >> 48)
	buf[pos+7] = uint8(x >> 56)
}

func (p *ProtoBuffer) ReadFixed64() uint64 {
	buf := p.buf
	pos := p.pos
	p.AddPos(SIZE_64BIT)

	if p.pos >= len(buf) {
		return 0
	}

	ret_low := uint32(buf[pos]) |
		(uint32(buf[pos+1]) << 8) |
		(uint32(buf[pos+2]) << 16) |
		(uint32(buf[pos+3]) << 24)
	ret_high := uint32(buf[pos+4]) |
		(uint32(buf[pos+5]) << 8) |
		(uint32(buf[pos+6]) << 16) |
		(uint32(buf[pos+7]) << 24)
	return (uint64(ret_high) << 32) | uint64(ret_low)
}

//float32
func (p *ProtoBuffer) WriteFloat32(f float32) {
	p.WriteFixed32(*(*uint32)(unsafe.Pointer(&f)))
}

func (p *ProtoBuffer) ReadFloat32() float32 {
	x := p.ReadFixed32()
	return *(*float32)(unsafe.Pointer(&x))
}

//float64
func (p *ProtoBuffer) WriteFloat64(f float64) {
	p.WriteFixed64(*(*uint64)(unsafe.Pointer(&f)))
}

func (p *ProtoBuffer) ReadFloat64() float64 {
	x := p.ReadFixed64()
	return *(*float64)(unsafe.Pointer(&x))
}

//boolean
func (p *ProtoBuffer) WriteBoolean(b bool) {
	if b {
		p.WriteInt32(1)
	} else {
		p.WriteInt32(0)
	}
}

func (p *ProtoBuffer) ReadBoolean() bool {
	x := p.ReadInt32()
	return x == 1
}

//string
func (p *ProtoBuffer) WriteString(s string) {
	l := len(s)
	p.WriteUInt32(uint32(l))
	copy(p.buf[p.pos:], s)
	p.AddPos(l)
}

func (p *ProtoBuffer) ReadString() string {
	l := p.ReadUInt32()
	old_pos := p.pos
	p.AddPos(int(l))
	s := string(p.buf[old_pos:p.pos])
	return s
}

//[]byte
func (p *ProtoBuffer) WriteBytes(b []byte) {
	l := len(b)
	p.WriteUInt32(uint32(l))
	copy(p.buf[p.pos:], b)
	p.AddPos(l)
}

func (p *ProtoBuffer) ReadBytes() []byte {
	l := p.ReadUInt32()
	old_pos := p.pos
	p.AddPos(int(l))

	b := make([]byte, l)
	copy(b, p.buf[old_pos:p.pos])
	return b
}

func (p *ProtoBuffer) GetUnknowFieldValueSize(wire_tag int32) {
	wire_type := wire_tag & 0x7

	switch wire_type {
	case WIRE_TYPE_VARINT:
		p.ReadUInt32()
	case WIRE_TYPE_64BIT:
		p.ReadFixed64()
	case WIRE_TYPE_LENGTH:
		size := p.ReadInt32()
		if size > 0 {
			p.AddPos(int(size))
		}
	case WIRE_TYPE_32_BIT:
		p.ReadFixed32()
	}
}

//error
type ProtoError struct {
	What string
}

func (e *ProtoError) Error() string {
	return e.What
}

//proto interface
type Message interface {
	Serialize(b *ProtoBuffer) error
	Parse(b *ProtoBuffer, msg_size int) error
	Clear()
	ByteSize() int
	IsInitialized() bool
	Print()
}
