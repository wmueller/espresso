#!/bin/sh
# tricking... the line after a these comments are interpreted as standard shell script \
    PLATFORM=`uname -s`; if [ "$1" != "" ]; then NP=$1; else NP=2; fi
# OSF1 \
    if test $PLATFORM = OSF1; then  exec dmpirun -np $NP $ESPRESSO_SOURCE/$PLATFORM/Espresso $0 $*
# AIX \
    elif test $PLATFORM = AIX; then exec poe $ESPRESSO_SOURCE/$PLATFORM/Espresso $0 $* -procs $NP
# Linux \
    else export EF_ALLOW_MALLOC_0=1; exec mpirun -np $NP -nsigs $ESPRESSO_SOURCE/$PLATFORM/Espresso $0 $*;
# \
    fi;

set epsilon 1e-4
setmd temp 0
setmd time_step 1
setmd skin 0

if { [setmd n_nodes] != 1 && [setmd n_nodes] != 2 &&
     [setmd n_nodes] != 4 && [setmd n_nodes] != 8 } {
    puts "Testcase dh.tcl does not run on odd numbers of nodes"
    exit 0
}

proc read_data {file} {
    set f [open $file "r"]
    while {![eof $f]} { blockfile $f read auto}
    close $f
}

proc write_data {file} {
    global energy pressure
    set f [open $file "w"]
    set energy [analyze energy total]
    set pressure [analyze pressure total]
    blockfile $f write tclvariable {energy pressure}
    blockfile $f write variable box_l
    blockfile $f write particles {id pos q f}
    close $f
}

if { [catch {
    read_data "dh_system.data"

    for { set i 0 } { $i <= [setmd max_part] } { incr i } {
	set F($i) [part $i pr f]
    }
    # to ensure force recalculation
    invalidate_system

    ############## dh-specific part, real space calculation

    setmd periodic 0 0 0
    setmd box_l 40 40 40
    inter coulomb 1 dh 0 20
    inter 0 fene 0 1
    integrate 0

    # here you can create the necessary snapshot
    # write_data "dh_system.data"

    # ensures that no other forces are on
    set cureng [analyze energy coulomb 0 0]
    # tbrs
    set curprs [lindex [analyze pressure coulomb 0 0] 0]

    ############## end

    set toteng [analyze energy total]
    set totprs [analyze pressure total]

    if { [expr abs($toteng - $cureng)] > $epsilon } {
	error "system has unwanted energy contributions"
    }
    if { [expr abs($totprs - $curprs)] > $epsilon } {
	error "system has unwanted pressure contributions"
    }

    set rel_eng_error [expr abs(($toteng - $energy)/$energy)]
    puts "relative energy deviations: $rel_eng_error"
    if { $rel_eng_error > $epsilon } {
	error "relative energy error too large"
    }

    set rel_prs_error [expr abs(($totprs - $pressure)/$pressure)]
    puts "relative pressure deviations: $rel_prs_error"
    if { $rel_prs_error > $epsilon } {
	error "relative pressure error too large"
    }

    set maxdx 0
    set maxpx 0
    set maxdy 0
    set maxpy 0
    set maxdz 0
    set maxpz 0
    for { set i 0 } { $i <= [setmd max_part] } { incr i } {
	set resF [part $i pr f]
	set tgtF $F($i)
	set dx [expr abs([lindex $resF 0] - [lindex $tgtF 0])]
	set dy [expr abs([lindex $resF 1] - [lindex $tgtF 1])]
	set dz [expr abs([lindex $resF 2] - [lindex $tgtF 2])]

	if { $dx > $maxdx} {
	    set maxdx $dx
	    set maxpx $i
	}
	if { $dy > $maxdy} {
	    set maxdy $dy
	    set maxpy $i
	}
	if { $dz > $maxdz} {
	    set maxdz $dz
	    set maxpz $i
	}
    }
    puts "maximal force deviation in x $maxdx for particle $maxpx, in y $maxdy for particle $maxpy, in z $maxdz for particle $maxpz"
    if { $maxdx > $epsilon || $maxdy > $epsilon || $maxdz > $epsilon } {
	if { $maxdx > $epsilon} {puts "force of particle $maxpx: [part $maxpx pr f] != $F($maxpx)"}
	if { $maxdy > $epsilon} {puts "force of particle $maxpy: [part $maxpy pr f] != $F($maxpy)"}
	if { $maxdz > $epsilon} {puts "force of particle $maxpz: [part $maxpz pr f] != $F($maxpz)"}
	error "force error too large"
    }

    ############## dh-specific part

    setmd box_l 40 40 40
    inter coulomb 1 dh 0 20
    integrate 0

    ############## end

    set maxdx 0
    set maxpx 0
    set maxdy 0
    set maxpy 0
    set maxdz 0
    set maxpz 0
    for { set i 0 } { $i <= [setmd max_part] } { incr i } {
	set resF [part $i pr f]
	set tgtF $F($i)
	set dx [expr abs([lindex $resF 0] - [lindex $tgtF 0])]
	set dy [expr abs([lindex $resF 1] - [lindex $tgtF 1])]
	set dz [expr abs([lindex $resF 2] - [lindex $tgtF 2])]

	if { $dx > $maxdx} {
	    set maxdx $dx
	    set maxpx $i
	}
	if { $dy > $maxdy} {
	    set maxdy $dy
	    set maxpy $i
	}
	if { $dz > $maxdz} {
	    set maxdz $dz
	    set maxpz $i
	}
    }
    puts "maximal force deviation in x $maxdx for particle $maxpx, in y $maxdy for particle $maxpy, in z $maxdz for particle $maxpz"
    if { $maxdx > $epsilon || $maxdy > $epsilon || $maxdz > $epsilon } {
	if { $maxdx > $epsilon} {puts "force of particle $maxpx: [part $maxpx pr f] != $F($maxpx)"}
	if { $maxdy > $epsilon} {puts "force of particle $maxpy: [part $maxpy pr f] != $F($maxpy)"}
	if { $maxdz > $epsilon} {puts "force of particle $maxpz: [part $maxpz pr f] != $F($maxpz)"}
	error "force error too large"
    }
} res ] } {
    puts "Error occured: \"$res\""
    exit -666
}

exit 0