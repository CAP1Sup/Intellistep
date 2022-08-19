export MotorSerial=/dev/ttyUSB0
export BaudRate=115200

export Negative=1
export Positive=0

openSerial() 
{
    stty -F ${MotorSerial} ${BaudRate}
    export LOGFILE=$(date +%y-%m-%d.%T).$(basename "$0").log
    echo '##### start of test' | tee ${LOGFILE}
    exec 4<${MotorSerial}
    exec 5>${MotorSerial}
}

closeSerial()
{
    echo "##### end of test" | tee -a ${LOGFILE}
    exec 4<&-
    exec 5<&-
}

sendGcode()
{    
    echo "##### $@" | tee -a ${LOGFILE}
    echo "<$@>" >&5
    readSerial "$@"
}


_sendGcodeTimed()
{
    sendGcode "$@"
    read
}

sendGcodeTimed()
{
    echo "press enter to start measure and enter at end of measure"
    read
    start=${EPOCHREALTIME}
    _sendGcodeTimed "$@" 
    stop=${EPOCHREALTIME}
    SpendTime=$(echo "${stop}-${start}" | bc )
}

readSerial()
{
    res=""
    _res="XXXX"
    while [ "$@ _END_" != "${_res}" ]; do
        read -r _res <&4
        if [ "$@ _END_" != "${_res}" ]; then
            if [ -z "${res}" ]; then
                res=${_res}
            else
                res=${res}$'\n'${_res}
            fi
        fi 
    done
    sleep 0.5
    echo "${res}" | tee -a ${LOGFILE}
    IFS=${OLD_IFS} 
}
