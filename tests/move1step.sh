source testconf.sh

nbOfSteps=1
rate=1
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
if [ ${res} != 1.80 ]; then exit 1; fi

cat <<EOF 
paramters :
- nbOfSteps : ${nbOfSteps}

motor configuration:
- microstepping : ${microstepping}

EOF


#direct stepping move 
sendGcode "G6 D${Positive} R${rate} S${nbOfSteps}"
echo "verify that motor turned ${nbOfSteps} in one direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi

#move same distance in opposite direction
sendGcode "G6 D${Negative} R${rate} S${nbOfSteps}"
echo "verify that motor turned ${nbOfSteps} opposite direction (y/n)"
read Answer
if [ "${Answer}" != "y" ]; then exit 1; fi
