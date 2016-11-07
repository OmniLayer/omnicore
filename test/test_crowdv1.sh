#!/bin/bash

PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
./src/omnicored --regtest --server --daemon --omniactivationallowsender=any --omni_debug=all >nul
sleep 10
printf "   * Preparing some mature testnet BTC\n"
./src/omnicore-cli --regtest setgenerate true 102 >null
printf "   * Obtaining a master address to work with\n"
ADDR=$(./src/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
./src/omnicore-cli --regtest sendtoaddress $ADDR 20 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":4}"
./src/omnicore-cli --regtest sendmany OMNIAccount $JSON >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Generating addresses to use\n"
ADDRESS=()
for i in {1..10}
do
   ADDRESS=("${ADDRESS[@]}" $(./src/omnicore-cli --regtest getnewaddress))
done
printf "\nTesting a crowdsale using BTC before activation...\n"
printf "   * Creating an indivisible test property and opening a crowdsale\n"
TXID=$(./src/omnicore-cli --regtest omni_sendissuancecrowdsale $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 0 10 1477488310 0 0)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the transaction was invalid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nActivating v2 crowdsales (allow BTC)... \n"
printf "   * Sending the activation\n"
BLOCKS=$(./src/omnicore-cli --regtest getblockcount)
TXID=$(./src/omnicore-cli --regtest omni_sendactivation $ADDR 11 $(($BLOCKS + 8)) 999)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the activation transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
./src/omnicore-cli --regtest setgenerate true 10 >null
printf "     # Checking the activation went live as expected... "
FEATUREID=$(./src/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c27-28)
if [ $FEATUREID == "11" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "\nTesting crowdsales using BTC after activation...\n"
printf "\nTesting an indivisible BTC crowdsale...\n"
printf "   * Creating an indivisible test property and opening a crowdsale\n"
CROWDTXID=$(./src/omnicore-cli --regtest omni_sendissuancecrowdsale $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 0 1000 1477488310 0 0)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $CROWDTXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting sending a BTC payment to the crowdsale...\n"
printf "   * Sending some BTC to %s\n" ${ADDRESS[1]}
./src/omnicore-cli --regtest sendtoaddress ${ADDRESS[1]} 0.2 >null
printf "   * Sending some BTC from %s to the crowdsale\n" ${ADDRESS[1]}
TXID=$(./src/omnicore-cli --regtest omni_sendbtcpayment ${ADDRESS[1]} $ADDR $CROWDTXID 0.1)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment was linked to the correct transaction... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep $CROWDTXID | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment was linked to the correct recipient... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep recipient | grep $ADDR | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment amount was 0.1 BTC... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -c22-)
if [ $RESULT == "\"0.10000000\"," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the sending address now has 100 of property 3... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[1]} 3 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "100" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking the transaction is listed in crowdsale participants... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 3 true | grep $TXID | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the crowdsale amount raised is now 0.1 BTC... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 3 true | grep amountraised | cut -d '"' -f4)
if [ $RESULT == "0.10000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the amount of tokens issued to users is now 100... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 3 true | grep tokensissued | cut -d '"' -f4)
if [ $RESULT == "100" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the amount of tokens issued to uthe isuser is still 0... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 3 true | grep addedissuertokens | cut -d '"' -f4)
if [ $RESULT == "0" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "\nTesting a divisible BTC crowdsale...\n"
printf "   * Sending some BTC to %s\n" ${ADDRESS[3]}
./src/omnicore-cli --regtest sendtoaddress ${ADDRESS[3]} 0.2 >null
printf "   * Sending some BTC to %s\n" ${ADDRESS[4]}
./src/omnicore-cli --regtest sendtoaddress ${ADDRESS[4]} 0.2 >null
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "   * Creating a divisible test property and opening a crowdsale\n"
CROWDTXID=$(./src/omnicore-cli --regtest omni_sendissuancecrowdsale ${ADDRESS[3]} 1 2 0 "Z_TestCat" "Z_TestSubCat" "Z_DivisTestProperty" "Z_TestURL" "Z_TestData" 0 1000 1477488310 0 0)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $CROWDTXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending some BTC from %s to the crowdsale\n" ${ADDRESS[4]}
TXID=$(./src/omnicore-cli --regtest omni_sendbtcpayment ${ADDRESS[4]} ${ADDRESS[3]} $CROWDTXID 0.1)
./src/omnicore-cli --regtest setgenerate true 1 >null
printf "     # Checking the transaction was valid... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment was linked to the correct transaction... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep $CROWDTXID | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment was linked to the correct recipient... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep recipient | grep ${ADDRESS[3]} | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the payment amount was 0.1 BTC... "
RESULT=$(./src/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -c22-)
if [ $RESULT == "\"0.10000000\"," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the sending address now has 100.0 of property 4... "
BALANCE=$(./src/omnicore-cli --regtest omni_getbalance ${ADDRESS[4]} 4 | grep balance | cut -d '"' -f4)
if [ $BALANCE == "100.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $BALANCE
    FAIL=$((FAIL+1))
fi
printf "     # Checking the transaction is listed in crowdsale participants... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 4 true | grep $TXID | wc -l)
if [ $RESULT == "1" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the crowdsale amount raised is now 0.1 BTC... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 4 true | grep amountraised | cut -d '"' -f4)
if [ $RESULT == "0.10000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the amount of tokens issued to users is now 100.0... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 4 true | grep tokensissued | cut -d '"' -f4)
if [ $RESULT == "100.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the amount of tokens issued to uthe isuser is still 0... "
RESULT=$(./src/omnicore-cli --regtest omni_getcrowdsale 4 true | grep addedissuertokens | cut -d '"' -f4)
if [ $RESULT == "0.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi

printf "\n"
printf "####################\n"
printf "#  Summary:        #\n"
printf "#    Passed = %d   #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

./src/omnicore-cli --regtest stop

