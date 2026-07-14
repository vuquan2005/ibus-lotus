/*
 * Bamboo - A Vietnamese Input method editor
 * Copyright (C) 2018 Luong Thanh Lam <ltlam93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"ibus-lotus/lotusibus"

	ibus "github.com/BambooEngine/goibus"
)

const (
	ComponentName = "org.freedesktop.IBus.lotus"
	EngineName    = "Lotus"
)

var embedded = flag.Bool("ibus", false, "Run the embedded ibus component")
var version = flag.Bool("version", false, "Show version")
var gui = flag.Bool("gui", false, "Show GUI")

func main() {
	flag.Parse()
	lotusibus.Embedded = *embedded
	lotusibus.ShowGUI = *gui
	lotusibus.Version = Version
	if *embedded {
		os.Chdir(lotusibus.DataDir)
	}
	if *version {
		fmt.Println(Version)
	} else if *embedded {
		engineCreator := lotusibus.GetIBusEngineCreator()
		bus := ibus.NewBus()
		bus.RequestName(ComponentName, 0)

		conn := bus.GetDbusConn()
		ibus.NewFactory(conn, engineCreator)

		go func() {
			<-conn.Context().Done()
			log.Println("DBus connection closed, exiting...")
			os.Exit(0)
		}()

		select {}
	} else {
		log.SetFlags(log.LstdFlags | log.Lmicroseconds)
		bus := ibus.NewBus()
		log.Println("Got Bus, Running Standalone")
		component := &ibus.Component{
			Name:          "IBusComponent",
			ComponentName: ComponentName + "Standalone",
		}
		engineDesc := &ibus.EngineDesc{
			Name:       "IBusEngineDesc",
			EngineName: EngineName + "Standalone",
		}
		component.AddEngine(engineDesc)
		bus.RegisterComponent(component)

		conn := bus.GetDbusConn()
		ibus.NewFactory(conn, lotusibus.GetIBusEngineCreator())

		bus.CallMethod("SetGlobalEngine", 0, EngineName+"Standalone")

		go func() {
			<-conn.Context().Done()
			log.Println("DBus connection closed, exiting...")
			os.Exit(0)
		}()

		select {}
	}
}

