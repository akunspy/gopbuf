package proto

import "testing"

func Test_Message(t *testing.T) {
	sub := NewSub()
	if sub.f1 != 8 || sub.f2 != 0 || sub.f3 != 0 {
		t.Error("Sub: test field default value error")
	}

	if sub.Hasf1() || sub.Hasf2() || sub.HasF3() {
		t.Error("Sub: test Has error")
	}

	sub.Setf2(5)
	if !sub.Hasf2() || sub.Getf2() != 5 {
		t.Error("Sub: test Set/Get error")
	}

	if sub.IsInitialized() {
		t.Error("Sub: test IsInitialized error")
	}
	sub.Setf1(1)
	if !sub.IsInitialized() {
		t.Error("Sub: test IsInitialized error")
	}

	buf := make([]byte, 100)
	err := sub.Serialize(buf)
	if err != nil {
		t.Error("Sub: test Serialize error")
	}

	newsub := NewSub()
	err = newsub.Parse(buf, sub.ByteSize())
	if err != nil {
		t.Error("Sub: test Parse error")
	}

	if sub.Getf1() != newsub.Getf1() ||
		sub.Getf2() != newsub.Getf2() ||
		sub.GetF3() != newsub.GetF3() {
		t.Error("Sub: test Parse error")
	}
	if !newsub.Hasf1() || !newsub.Hasf2() || newsub.HasF3() {
		t.Error("Sub: test Parse error")
	}

	sub.Clear()
	if sub.IsInitialized() {
		t.Error("Sub: test Clear error")
	}
	if sub.f1 != 8 || sub.f2 != 0 || sub.f3 != 0 {
		t.Error("Sub: test Clear error")
	}
	if sub.Hasf1() || sub.Hasf2() || sub.HasF3() {
		t.Error("Sub: test Clear error")
	}
}
