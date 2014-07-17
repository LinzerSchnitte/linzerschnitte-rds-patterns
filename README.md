linzerschnitte-rds-patterns
=======================

LinzerSchnitte MIDI to RDS interface for the Raspberry Pi and TX190 FM Transmitter

Install:
run ./make-midi

to use run as root ./midi2rds

This binary transforms incoming MIDI messages into RDS commands and issues them. Program Change messages trigger commands, Control Change messages change parameters for the various commands.

For a list of commands, consult http://www.aec.at/linzerschnitte/wiki/index.php/RDS
