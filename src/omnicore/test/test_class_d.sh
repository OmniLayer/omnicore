#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "##################################################################\n"
printf "# Performing Class D encoding and decoding scenarios via regtest #\n"
printf "##################################################################\n\n"
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
$SRCDIR/omnicored --regtest --server --daemon --omnidebug=all --omniactivationallowsender=any >$NUL
sleep 5
printf "   * Preparing some mature testnet BTC\n"
$SRCDIR/omnicore-cli --regtest generate 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 20 >$NUL
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
TOADD=$($SRCDIR/omnicore-cli --regtest getnewaddress)
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":4,\""$TOADD"\":4}"
$SRCDIR/omnicore-cli --regtest sendmany OMNIAccount $JSON >$NUL
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   * Creating an indivisible test property\n"
$SRCDIR/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "Z_TestCat" "Z_TestSubCat" "Z_IndivisTestProperty" "Z_TestURL" "Z_TestData" 10000000 >$NUL
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "\nActivating Class D transactions...\n"
printf "   * Sending the activation\n"
BLOCKS=$($SRCDIR/omnicore-cli --regtest getblockcount)
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendactivation $ADDR 13 $(($BLOCKS + 8)) 999)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "     # Checking the activation transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Mining 10 blocks to forward past the activation block\n"
$SRCDIR/omnicore-cli --regtest generate 10 >$NUL
printf "     # Checking the activation went live as expected... "
FEATUREID=$($SRCDIR/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c20-21)
if [ $FEATUREID == "13" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Simple Send' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_send $ADDR $TOADD 1 50.0)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "00000180e497d012" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "50.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Send To Owners' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendsto $ADDR 1 10.0)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "0003018094ebdc03" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "3," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "10.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Send All' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendall $ADDR $TOADD 2)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "000402" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "4," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the ecosystem... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep ecosystem | cut -d '"' -f4)
if [ $RESULT == "test" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'DEx Sell' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_senddexsell $ADDR 1 32.12345678 1.00000001 20 0.001 1)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "011401ce82e2fb0b81c2d72f14a08d0601" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "20," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "1," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyid | cut -d ' ' -f4)
if [ $RESULT == "1," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "32.12345678" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep bitcoindesired | cut -d '"' -f4)
if [ $RESULT == "1.00000001" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payment window... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep timelimit | cut -d ' ' -f4)
if [ $RESULT == "20," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the minimum fee... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep feerequired | cut -d '"' -f4)
if [ $RESULT == "0.00100000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the action... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep action | cut -d '"' -f4)
if [ $RESULT == "new" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'DEx Accept' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_senddexaccept $TOADD $ADDR 1 15.0)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "00160180dea0cb05" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "22," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "15.00000000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'MetaDEx Trade' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendtrade $ADDR 1 11.99999999 3 60)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "001901ff979abc04033c" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "25," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyidforsale\" | cut -d ' ' -f4)
if [ $RESULT == "1," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amountforsale | cut -d '"' -f4)
if [ $RESULT == "11.99999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyiddesired\" | cut -d ' ' -f4)
if [ $RESULT == "3," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amountdesired | cut -d '"' -f4)
if [ $RESULT == "60" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'MetaDEx Cancel Price' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendcanceltradesbyprice $ADDR 1 11.99999999 3 60)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "001a01ff979abc04033c" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "26," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyidforsale\" | cut -d ' ' -f4)
if [ $RESULT == "1," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amountforsale | cut -d '"' -f4)
if [ $RESULT == "11.99999999" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyiddesired\" | cut -d ' ' -f4)
if [ $RESULT == "3," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amountdesired | cut -d '"' -f4)
if [ $RESULT == "60" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'MetaDEx Cancel Pair' for encoding and decoding...\n"
printf "   # Relisting a trade\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendtrade $ADDR 1 11.99999999 3 60)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendcanceltradesbypair $ADDR 1 3)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "001b0103" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "27," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property for sale... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyidforsale\" | cut -d ' ' -f4)
if [ $RESULT == "1," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyiddesired\" | cut -d ' ' -f4)
if [ $RESULT == "3," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'MetaDEx Cancel Ecosystem' for encoding and decoding...\n"
printf "   # Relisting a trade\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendtrade $ADDR 1 11.99999999 3 60)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendcancelalltrades $ADDR 1)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "001c01" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "28," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the ecosystem... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep \"ecosystem\" | cut -d '"' -f4)
if [ $RESULT == "main" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Create Property - Fixed' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendissuancefixed $ADDR 1 1 0 "TestCat" "TestSubCat" "TestProperty" "TestURL" "TestData" 14321)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "0032010100546573744361740054657374537562436174005465737450726f7065727479005465737455524c00546573744461746100f16f" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "50," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertytype | cut -d '"' -f4)
if [ $RESULT == "indivisible" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the ecosystem... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep ecosystem | cut -d '"' -f4)
if [ $RESULT == "main" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the category... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep \"category | cut -d '"' -f4)
if [ $RESULT == "TestCat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the subcategory... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep subcategory | cut -d '"' -f4)
if [ $RESULT == "TestSubCat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property name... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyname | cut -d '"' -f4)
if [ $RESULT == "TestProperty" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the data... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep data | cut -d '"' -f4)
if [ $RESULT == "TestData" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the URL... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep url | cut -d '"' -f4)
if [ $RESULT == "TestURL" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "14321" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Create Property - Variable' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendissuancecrowdsale $ADDR 1 1 0 "Cat" "SubCat" "Test" "URL" "Data" 3 400 1594309459 10 5)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "00330101004361740053756243617400546573740055524c004461746100039003d3f69cf8050a05" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "51," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertytype | cut -d '"' -f4)
if [ $RESULT == "indivisible" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the ecosystem... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep ecosystem | cut -d '"' -f4)
if [ $RESULT == "main" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the category... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep \"category | cut -d '"' -f4)
if [ $RESULT == "Cat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the subcategory... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep subcategory | cut -d '"' -f4)
if [ $RESULT == "SubCat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property name... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyname | cut -d '"' -f4)
if [ $RESULT == "Test" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the data... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep data | cut -d '"' -f4)
if [ $RESULT == "Data" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the URL... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep url | cut -d '"' -f4)
if [ $RESULT == "URL" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property ID desired... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyiddesired | cut -d ' ' -f4)
if [ $RESULT == "3," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the tokensperunit... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep tokensperunit | cut -d '"' -f4)
if [ $RESULT == "400" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the deadline... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep deadline | cut -d ' ' -f4)
if [ $RESULT == "1594309459," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the early bonus... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep earlybonus | cut -d ' ' -f4)
if [ $RESULT == "10," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the issuer percentage... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep percenttoissuer | cut -d ' ' -f4)
if [ $RESULT == "5," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Close Crowdsale' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendclosecrowdsale $ADDR 5)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "003505" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "53," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property ID... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyid | cut -d ' ' -f4)
if [ $RESULT == "5," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Create Property - Managed' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendissuancemanaged $ADDR 1 1 0 "TestCat" "TestSubCat" "TestProperty" "TestURL" "TestData")
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "0036010100546573744361740054657374537562436174005465737450726f7065727479005465737455524c00546573744461746100" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "54," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertytype | cut -d '"' -f4)
if [ $RESULT == "indivisible" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the ecosystem... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep ecosystem | cut -d '"' -f4)
if [ $RESULT == "main" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the category... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep \"category | cut -d '"' -f4)
if [ $RESULT == "TestCat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the subcategory... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep subcategory | cut -d '"' -f4)
if [ $RESULT == "TestSubCat" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property name... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyname | cut -d '"' -f4)
if [ $RESULT == "TestProperty" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the data... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep data | cut -d '"' -f4)
if [ $RESULT == "TestData" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the URL... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep url | cut -d '"' -f4)
if [ $RESULT == "TestURL" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Grant' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendgrant $ADDR "" 6 25)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "0037061900" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "55," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "25" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Revoke' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendrevoke $ADDR 6 15)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "0038060f00" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "56," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the amount... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep amount | cut -d '"' -f4)
if [ $RESULT == "15" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
#-------------------------------------------------------------------
#-------------------------------------------------------------------
printf "\nTesting a Class D 'Change Issuer' for encoding and decoding...\n"
printf "   # Sending the transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendchangeissuer $ADDR $TOADD 6)
$SRCDIR/omnicore-cli --regtest generate 1 >$NUL
printf "   # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c12-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the payload is as expected... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getpayload $TXID | grep payload\" | cut -d '"' -f4)
if [ $RESULT == "004606" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction type... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep type_int | cut -d ' ' -f4)
if [ $RESULT == "70," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the transaction version... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep version | cut -d ' ' -f4)
if [ $RESULT == "0," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   # Checking the property ID... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep propertyid | cut -d ' ' -f4)
if [ $RESULT == "6," ]
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
printf "#    Passed = %d  #\n" $PASS
printf "#    Failed = %d    #\n" $FAIL
printf "####################\n"
printf "\n"

$SRCDIR/omnicore-cli --regtest stop
exit

