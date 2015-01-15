#! /bin/bash

#****************************************************************************
#* This file is free software: you can redistribute it and/or modify        *
#* it under the terms of the GNU General Public License as published by     *
#* the Free Software Foundation, either version 3 of the License, or        *
#* (at your option) any later version.                                      *
#*                                                                          *
#* Primary Authors: Matthias Richter <Matthias.Richter@scieq.net>           *
#*                                                                          *
#* The authors make no claims about the suitability of this software for    *
#* any purpose. It is provided "as is" without express or implied warranty. *
#****************************************************************************

#  @file   cluster-relay-to-tracker.sh
#  @author Matthias Richter
#  @since  2014-11-26 
#  @brief  Launch script for ALICE HLT TPC processing topology


###################################################################
# global settings
runno=167808

# note: don't want to overdo the logic now, the lastslice should be firstslice plus multiples of slices_per_node minus 1
# lastslice=(firstslice + n*slices_per_node) - 1
#
# TODO: this possibly needs some more checking 
firstslice=0
lastslice=35
slices_per_node=4
#dryrun="-n"
pollingtimeout=100
rundir=`pwd`

baseport_on_flpgroup=48400
baseport_on_epn1group=48450
flp_command_socket=48490
flp_heartbeat_socket=48491
number_of_epns=12

# uncomment the following line to print the commands instead of
# actually launching them
#printcmdtoscreen='echo'

# uncomment the following line to bypass the FLP relays and
# send data directly to th tracker
#bypass_relays=yes

# uncomment the following line to bypass the tracking
#bypass_tracking=yes

###################################################################
# argument scan
# TODO: implement real argument scan and more configurable options
while [ "x$1" != "x" ]; do
    if [ "x$1" == "x--print-commands" ]; then
	printcmdtoscreen='echo'
    fi
    shift
done

###################################################################
######## end of configuration area                     ############
######## no changes beyond this point                  ############
###################################################################

###################################################################
# fill the list of nodes from subsidiary script in the current
# directory
# 
nodelist=
nodelistfile=nodelist.sh
if [ -e $nodelistfile ]; then
    . $nodelistfile
fi

if [ "x$nodelist" == "x" ]; then
cat <<EOF
error: can not find node definition

Please add a script file 'nodelist.sh' in the current directory
defining the nodes to be used in this topology.

############## example ##############################
nodelist=
nodelist=(\${nodelist[@]} localhost)
#nodelist=(\${nodelist[@]} someothernode)

EOF
exit -1
fi
nnodes=${#nodelist[@]}
echo "using $nnodes node(s) for running processing topology"
echo ${nodelist[@]}

# init the variables for the session commands
sessionnode=
sessiontitle=
sessioncmd=
nsessions=0


###################################################################
# check if the output of the parent is binding or not
# and set the attributes according to that
# --output --> --input
# bind <--> connect
# push <--> pull
translate_io_attributes() {
    __inputattributes=$1
    __translated=`echo $1 | sed -e 's|--output|--input|g'`
    __node=${2:-localhost}
    if [ x`echo ${__inputattributes} | sed -e 's|.*method=\(.*\)|\1|' -e 's|,.*$||'` == "xbind" ]; then
	__translated=`echo ${__translated} | sed -e "s|://\*:|://${__node}:|g" -e 's|method=bind|method=connect|g'`
    else
	__translated=`echo ${__translated} | sed -e 's|://.*:|://*:|g' -e 's|method=connect|method=bind|g'`
    fi
    __translated=`echo ${__translated} | sed -e 's/type=push/type=pull/g'`
    echo $__translated
}

translate_io_alternateformat() {
    __inputattributes=$1
    io=`echo $__inputattributes | sed -e 's| .*$||`
    type=`echo $__inputattributes | sed -e 's| .*$||`
    io=`echo $__inputattributes | sed -e 's| .*$||`
}

###################################################################
# create an flp node group
epn1_input=
n_epn1_inputs=0
create_flpgroup() {
    node=$1
    basesocket=$2
    firstslice_on_node=$3
    nofslices=$4
    cf_output=
    for ((c=0; c<nofslices; c++)); do
	slice=$((firstslice_on_node + c))
	socket=$((basesocket + c))
	spec=`printf %02d $slice`
	output="--output type=push,size=5000,method=bind,address=tcp://*:$socket"
	command="aliceHLTWrapper ClusterPublisher_$spec 1 --poll-period $pollingtimeout $output --library libAliHLTUtil.so --component FilePublisher --run $runno --parameter '-datafilelist data/emulated-tpc-clusters_slice_$spec.txt'"
	deviceid="ClusterPublisher_$spec"

	sessionnode[nsessions]=$node
	sessiontitle[nsessions]=$deviceid
	sessioncmd[nsessions]=$command
	let nsessions++

	cf_output="$cf_output ${output}"
    done

    if [ "x$bypass_relays" != "xyes" ]; then
        # add a relay combining CF output into one (multi)message
        deviceid="Relay_$node"
        input=`translate_io_attributes "$cf_output"`
        output="--output type=push,size=5000,method=bind,address=tcp://*:$((basesocket + c))"
        let socketcount++
        command="aliceHLTWrapper $deviceid 1 ${dryrun} --poll-period $pollingtimeout $input $output --library libAliHLTUtil.so --component BlockFilter --run $runno --parameter ''"

        sessionnode[nsessions]=$node
        sessiontitle[nsessions]="$deviceid"
        sessioncmd[nsessions]=$command
        let nsessions++

        deviceid="FLP_$node"
	command="testFLP_distributed"
	command+=" --id $deviceid"
	command+=" --num-inputs 3"
	command+=" --num-outputs $number_of_epns"
	command+=" --heartbeat-timeout 20000"
	command+=" --send-offset 0" #$((10#$i))"
	command+=" --input-socket-type sub --input-buff-size 500 --input-method bind --input-address tcp://*:$flp_command_socket   --log-input-rate 0" # command input
	command+=" --input-socket-type sub --input-buff-size 500 --input-method bind --input-address tcp://*:$flp_heartbeat_socket --log-input-rate 0" # heartbeat input
	command+=" "
        input=`translate_io_attributes "$output"`
	#command+=`translate_io_alternateformat $input` # data input
	command+=" --input-socket-type pull --input-buff-size 5000 --input-method connect --input-address tcp://$node:$((basesocket + c)) --log-input-rate 1" # data input
	for ((j=0; j<$number_of_epns; j++));
	do
            command+=" --output-socket-type push --output-buff-size 5000 --output-method connect --output-address tcp://10.162.130.79:$((j + 48480)) --log-output-rate 1"
#            output="--output type=push,size=5000,method=connect,address=tcp://cn59-ib:48480$j"
#            command+=" "
#            command+=`translate_io_alternateformat $output`
#            command+=" --log-output-rate 1"
#            epn1_input[n_epn1_inputs]=${output/\/\/\*:///$node:}
#            let n_epn1_inputs++
	done
        epn1_input[n_epn1_inputs]=${node}
        let n_epn1_inputs++

	sessionnode[nsessions]=$node
	sessiontitle[nsessions]=$deviceid
	sessioncmd[nsessions]=$command
	let nsessions++
    else
        # add each CF output directly to EPN input
	# TODO: this is a bug, but it does not harm
	# the string is broken up at blanks, so the option-parameter
	# relation is lost and there are too many elements in the array,
	# the array is expanded to a string, so does not matter at the moment
        for output in $cf_output; do
            epn1_input[n_epn1_inputs]=${output/\/\/\*:///$node:}
            let n_epn1_inputs++
        done
    fi
}

###################################################################
# create an epn1 node group
create_epn1group() {
    node=$1
    basesocket=$2
    socketcount=0

    output=`echo "${epn1_input[@]}"`
    if [ "x$bypass_tracking" != "xyes" ]; then
	for ((trackerid=0; trackerid<$number_of_epns; trackerid++)); do
            deviceid=EPN_`printf %02d $trackerid`
	    command="testEPN_distributed"
	    command+=" --id $deviceid"
	    command+=" --num-outputs $((n_epn1_inputs + 1))"
	    command+=" --heartbeat-interval 5000"
	    command+=" --buffer-timeout 30000"
	    command+=" --num-flps $n_epn1_inputs"
	    command+=" --input-socket-type pull --input-buff-size 5000 --input-method bind --input-address tcp://10.162.130.79:$((trackerid + 48480)) --log-input-rate 1" # data input
	    for flpnode in ${epn1_input[@]}; # heartbeats
	    do
		command+=" --output-socket-type pub --output-buff-size 500 --output-method connect --output-address tcp://$flpnode:$flp_heartbeat_socket --log-output-rate 0"
	    done
	    command+=" --output-socket-type push --output-buff-size 5000 --output-method bind --output-address tcp://*:$((basesocket + socketcount)) --log-output-rate 1" # data output
        
	sessionnode[nsessions]=$node
        sessiontitle[nsessions]="$deviceid"
        sessioncmd[nsessions]=$command
        let nsessions++

        deviceid=Tracker_`printf %02d $trackerid`
	#output=`echo "${epn1_input[@]}"`
        #input=`translate_io_attributes "$output"`
	input="--input type=pull,size=1000,method=connect,address=tcp://localhost:$((basesocket + socketcount))"
	let socketcount++
        output="--output type=push,size=1000,method=connect,address=tcp://localhost:$((basesocket + number_of_epns))"
        command="aliceHLTWrapper $deviceid 1 ${dryrun} --poll-period $pollingtimeout $input $output --library libAliHLTTPC.so --component TPCCATracker --run $runno --parameter '-GlobalTracking -loglevel=0x79'"

        sessionnode[nsessions]=$node
        sessiontitle[nsessions]="$deviceid"
        sessioncmd[nsessions]=$command
        let nsessions++
	done
        socketcount=$((trackerid + 1))

        deviceid=GlobalMerger
        input=`translate_io_attributes "$output"`
        output="--output type=push,size=1000,method=bind,address=tcp://*:$((basesocket + socketcount))"
        let socketcount++
        command="aliceHLTWrapper $deviceid 1 ${dryrun} --poll-period $pollingtimeout $input $output --library libAliHLTTPC.so --component TPCCAGlobalMerger --run $runno --parameter '-loglevel=0x7c'"

        sessionnode[nsessions]=$node
        sessiontitle[nsessions]="$deviceid"
        sessioncmd[nsessions]=$command
        let nsessions++
    fi

    deviceid=FileWriter
    input=`translate_io_attributes "$output"`
    output=
    let socketcount++
    command="aliceHLTWrapper $deviceid 1 ${dryrun} --poll-period $pollingtimeout $input $output --library libAliHLTUtil.so --component FileWriter --run $runno --parameter '-directory tracker-output -idfmt=%04d -specfmt=_%08x -blocknofmt= -loglevel=0x7c -write-all-blocks -publisher-conf tracker-output/datablocks.txt'"

    sessionnode[nsessions]=$node
    sessiontitle[nsessions]="$deviceid"
    sessioncmd[nsessions]=$command
    let nsessions++
}

########### main script ###########################################
#
# now build the commands on the nodes
# flp nodegroups
sliceno=$firstslice
inode=0
while [ "$sliceno" -le "$lastslice" ]; do
    create_flpgroup ${nodelist[inode]} $baseport_on_flpgroup $sliceno $slices_per_node
    sliceno=$((sliceno + slices_per_node))

    let inode++
    if [ "$inode" -ge "$nnodes" ]; then
	echo "error: too few nodes to create all flp node groups"
	sliceno=$((lastslice + 1))
    fi
done

# epn1 nodegroup
if [ "$inode" -lt "$nnodes" ]; then
    # epn1 group on the last node
    inode=$((nnodes - 1))
    create_epn1group ${nodelist[inode]} $baseport_on_epn1group 
else
    echo "error: too few nodes to create the epn1 node group"
    exit -1
fi

# start the screen sessions and devices
applications=
for ((isession=$nsessions++-1; isession>=0; isession--)); do
    if [ "x$printcmdtoscreen" == "x" ]; then
        echo "starting ${sessiontitle[$isession]} on ${sessionnode[$isession]}: ${sessioncmd[$isession]}"
    fi
    #logcmd=" 2>&1 | tee ${sessiontitle[$isession]}.log"
    $printcmdtoscreen screen -d -m -S "${sessiontitle[$isession]} on ${sessionnode[$isession]}" ssh ${sessionnode[$isession]} "(cd $rundir && source setup.sh && ${sessioncmd[$isession]}) $logcmd" &
    applications+=" "`echo ${sessioncmd[$isession]} | sed -e 's| .*$||'`
    # sleep between starts, some of the screens are not started if the frequency is too high
    usleep 200000
done

if [ "x$printcmdtoscreen" == "x" ]; then
usednodes=`for n in ${sessionnode[@]}; do echo $n; done | sort | uniq`
echo
echo "started processing topology in ${#sessionnode[@]} session(s) on `echo $usednodes | wc -w` node(s):"
usednodes=`echo $usednodes | sed ':a;N;$!ba;s/\n/ /g'`
echo $usednodes

applications=`for app in $applications; do echo $app; done | sort | uniq`
echo
echo "a simple method to stop the devices:"
for app in $applications; do
    echo "for node in $usednodes; do ssh \$node killall $app; done"
done
fi
