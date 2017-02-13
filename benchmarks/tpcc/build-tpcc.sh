backend=$1 # e.g: herwl
htm_retries=$2 # e.g.: 5
rot_retries=$3 # e.g.: 2 

cp ../../backends/$backend/tm.h code/
cp ../../backends/$backend/thread.c code/
cp ../../backends/$backend/thread.h code/
cp ../../backends/$backend/Makefile . 
cp ../../backends/$backend/Makefile.common .
cp ../../backends/$backend/Makefile.flags .
cp ../../backends/$backend/Defines.common.mk . 

cd code;
rm tpcc

if [[ $backend == htm-sgl || $backend == hybrid-norec ]]; then
	make_command="make HTM_RETRIES=-DHTM_RETRIES=$htm_retries RETRY_POLICY=-DRETRY_POLICY=$rot_retries"
else
	make_command="make HTM_RETRIES=-DHTM_RETRIES=$htm_retries ROT_RETRIES=-DROT_RETRIES=$rot_retries"
fi
$make_command
