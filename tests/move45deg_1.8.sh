source testconf.sh

RequestedAngle=45
MotorFullStepAngle=1.80
#speed in mm/s
rate=20

#uncomment following line if you want to activate trace mode 
#set -x
openSerial

#get firmware information
sendGcode "M115"

#get firmware configuration
sendGcode "M503"

#set microstepping divisor
sendGcode "M350 S1"
#get microstepping divisor
sendGcode "M350"
microstepping=${res}

#enable motor
sendGcode "M17"

#get angle information
sendGcode "M93 V${MotorFullStepAngle}"
sendGcode "M93"
if [ ${res} != ${MotorFullStepAngle} ]; then exit 1; fi
nbOfSteps=$(echo "${RequestedAngle}/(${MotorFullStepAngle}*${microstepping})" | bc)


cat <<EOF 
#########################################
paramters :
- Requested Angle : ${RequestedAngle} deg
- Motor full step angle : ${MotorFullStepAngle} deg
- rate : ${rate}

motor configuration:
- microstepping : ${microstepping}

computed :
- nb of steps : ${nbOfSteps}

EOF

#direct stepping move 
sendGcode "G6 D${Negative} R${rate} S${nbOfSteps}"
echo "verify that ${RequestedAngle} degrees was traveled in one direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi

#move same distance in opposite direction
sendGcode "G6 D${Positive} R${rate} S${nbOfSteps}"
echo "verify that ${RequestedAngle} degrees was traveled in opposite direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi
