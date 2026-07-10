package main

import (
	"fmt"
	"ibus-bamboo/config"
	"log"
	"time"
)

const BACKSPACE_INTERVAL = 0

var fakeBackspaceCount = 0

func (e *IBusBambooEngine) SendBackSpace(n int) {
	fakeBackspaceCount = n
	e.SendBackspaceFromInputMode()
}

func (e *IBusBambooEngine) SendBackspaceFromInputMode() {
	switch e.getInputMode() {
	case config.SurroundingTextIM:
		e.SendBackspaceInSurroundingTextMode()
	default:
		fmt.Println("There's something wrong with wmClasses")
	}
}

func (e *IBusBambooEngine) SendBackspaceInSurroundingTextMode() {
	time.Sleep(20 * time.Millisecond)
	log.Printf("Sendding %d backspace via SurroundingText\n", fakeBackspaceCount)
	e.DeleteSurroundingText(-int32(fakeBackspaceCount), uint32(fakeBackspaceCount))
	time.Sleep(20 * time.Millisecond)
}

func (e *IBusBambooEngine) resetFakeBackspace() {
	fakeBackspaceCount = 0
}
