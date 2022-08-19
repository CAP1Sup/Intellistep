source testconf.sh

mmPerTurn=8
distanceToTravel_mm=50
#speed in mm/s
speed=20

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
sendGcode "M93"
if [ ${res} != 1.80 ]; then echo "wrong angle setting"; exit 1; fi
stepPerTurn=$(echo "(360*${microstepping})/${res}" | bc)
stepPermm=$(echo "${stepPerTurn}/${mmPerTurn}" | bc)
nbOfSteps=$(echo "${stepPermm}*${distanceToTravel_mm}" | bc)
duration_s=$(echo "${distanceToTravel_mm}/${speed}" | bc)
rate=$(echo "${nbOfSteps}/${duration_s}" | bc)

cat <<EOF 
paramters :
- mm per turn : ${mmPerTurn}
- distance to travel : ${distanceToTravel_mm}
- speed : ${speed} mm/s

motor configuration:
- microstepping : ${microstepping}

computed :
- step per turn : ${stepPerTurn}
- step per mm : ${stepPermm}
- nb of steps : ${nbOfSteps}
- rate : ${rate}
EOF


#direct stepping move 
sendGcodeTimed "G6 D${Positive} R${rate} S${nbOfSteps}"
SpeedMeasure=$(echo "${distanceToTravel_mm}.0/${SpendTime}" | bc)
echo "measured Speed = ${SpeedMeasure}"
echo "verify that ${distanceToTravel_mm}mm was traveled in one direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi

#move same distance in opposite direction
sendGcodeTimed "G6 D${Negative} R${rate} S${nbOfSteps}"
SpeedMeasure=$(echo "${distanceToTravel_mm}.0/${SpendTime}" | bc)
echo "measured Speed = ${SpeedMeasure}"
echo "verify that ${distanceToTravel_mm}mm was traveled in opposite direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi

closeSerial