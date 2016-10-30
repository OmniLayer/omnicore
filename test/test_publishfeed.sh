#!/bin/bash

SRCDIR=./src/
NUL=/dev/null
PASS=0
FAIL=0
clear
printf "Preparing a test environment...\n"
printf "   * Starting a fresh regtest daemon\n"
rm -r ~/.bitcoin/regtest
$SRCDIR/omnicored --regtest --server --daemon --omniactivationallowsender=any >$NUL
sleep 10
printf "   * Preparing some mature testnet BTC\n"
$SRCDIR/omnicore-cli --regtest setgenerate true 102 >$NUL
printf "   * Obtaining a master address to work with\n"
ADDR=$($SRCDIR/omnicore-cli --regtest getnewaddress OMNIAccount)
printf "   * Funding the address with some testnet BTC for fees\n"
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 0.001 >$NUL
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 0.002 >$NUL
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 0.003 >$NUL
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 0.004 >$NUL
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 0.005 >$NUL
$SRCDIR/omnicore-cli --regtest sendtoaddress $ADDR 10.005 >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Participating in the Exodus crowdsale to obtain some OMNI\n"
JSON="{\"moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP\":10,\""$ADDR"\":0.0002}"
$SRCDIR/omnicore-cli --regtest sendmany OMNIAccount $JSON >$NUL
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Master address is %s\n" $ADDR
printf "   * Generating addresses to use\n"
ADDRESS=()
for i in {1..6}
do
   ADDRESS=("${ADDRESS[@]}" $($SRCDIR/omnicore-cli --regtest getnewaddress))
done
printf "Confirming publish feed transactions are invalidated before activation...\n"
printf "   * Sending a Publish Feed transaction\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 1000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the transaction was invalid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "false," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "Activating the publish feed transaction...\n"
printf "   * Sending the activation\n"
BLOCKS=$($SRCDIR/omnicore-cli --regtest getblockcount)
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendactivation $ADDR 12 $(($BLOCKS + 8)) 1100001)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the activation transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Mining 10 blocks to forward past the activation block\n"
$SRCDIR/omnicore-cli --regtest setgenerate true 10 >$NUL
printf "     # Checking the activation went live as expected... "
FEATUREID=$($SRCDIR/omnicore-cli --regtest omni_getactivations | grep -A 10 completed | grep featureid | cut -c27-28)
if [ $FEATUREID == "12" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $FEATUREID
    FAIL=$((FAIL+1))
fi
printf "Testing Publish Feed...\n"
printf "   * Sending a Publish Feed transaction (feed 1, value 1000)\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 1000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the transaction was valid... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_gettransaction $TXID | grep valid | cut -c15-)
if [ $RESULT == "true," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 1 at address is now 1000... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Getting the block number to use later...\n"
BLOCKS=$($SRCDIR/omnicore-cli --regtest getblockcount)
printf "   * Sending several Publish Feed transactions for feed 1, (highest will be 5000 but latest 3000)\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 2000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 4000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 5000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 3000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the value for feed 1 at address is now at latest (3000)... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "3000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 1 at address is 1000 at block %d... " $BLOCKS
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 $BLOCKS | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Sending Publish Feed transactions for feed refernces 2 and 65\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 2 200)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 65 6500)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the value for feed 2 at address is now 200... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 2 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "200" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking the value for feed 65 at address is now 6500... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 65 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "6500" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "     # Checking that feeds 1, 2 and 65 are returned in omni_getfeeds... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeeds $ADDR | grep reference | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "1,2,65," ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Checking feed history for feed ref 1\n"
printf "     # Checking that values 1000, 2000, 4000, 5000 & 3000 exist in history... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeedhistory $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "10002000400050003000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Committing feed database to log\n"
$SRCDIR/omnicore-cli --regtest mscrpc 15 >$NUL
printf "Orphaning a block to test Feed DB reorg protection (disconnecting 1 block from tip and mining a replacement)\n"
printf "     # Checking the value for feed 1 at address is still at 3000... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "3000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Updating the feed to 9000\n"
TXID=$($SRCDIR/omnicore-cli --regtest omni_sendpublishfeed $ADDR 1 9000)
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "     # Checking the value for feed 1 at address is now at 9000... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "9000" ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $RESULT
    FAIL=$((FAIL+1))
fi
printf "   * Committing feed database to log\n"
$SRCDIR/omnicore-cli --regtest mscrpc 15 >$NUL
printf "   * Executing the rollback\n"
BLOCK=$($SRCDIR/omnicore-cli --regtest getblockcount)
BLOCKHASH=$($SRCDIR/omnicore-cli --regtest getblockhash $(($BLOCK)))
$SRCDIR/omnicore-cli --regtest invalidateblock $BLOCKHASH >$NUL
PREVBLOCK=$($SRCDIR/omnicore-cli --regtest getblockcount)
printf "   * Clearing the mempool\n"
$SRCDIR/omnicore-cli --regtest clearmempool >$NUL
printf "   * Verifiying the results\n"
printf "      # Checking the block count has been reduced by 1... "
EXPBLOCK=$((BLOCK-1))
if [ $EXPBLOCK == $PREVBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $PREVBLOCK
    FAIL=$((FAIL+1))
fi
printf "   * Mining a replacement block\n"
$SRCDIR/omnicore-cli --regtest setgenerate true 1 >$NUL
printf "   * Committing feed database to log\n"
$SRCDIR/omnicore-cli --regtest mscrpc 15 >$NUL
printf "   * Verifiying the results\n"
NEWBLOCK=$($SRCDIR/omnicore-cli --regtest getblockcount)
NEWBLOCKHASH=$($SRCDIR/omnicore-cli --regtest getblockhash $(($BLOCK)))
printf "      # Checking the block count is the same as before the rollback... "
if [ $BLOCK == $NEWBLOCK ]
  then
    printf "PASS\n"
    PASS=$((PASS+1))
  else
    printf "FAIL (result:%s)\n" $NEWBLOCK
    FAIL=$((FAIL+1))
fi
printf "      # Checking the block hash is different from before the rollback... "
if [ $BLOCKHASH == $NEWBLOCKHASH ]
  then
    printf "FAIL (result:%s)\n" $NEWBLOCKHASH
    FAIL=$((FAIL+1))
  else
    printf "PASS\n"
    PASS=$((PASS+1))
fi
printf "     # Checking the value for feed 1 at address has been rolled back to 3000... "
RESULT=$($SRCDIR/omnicore-cli --regtest omni_getfeed $ADDR 1 | grep value | cut -d ':' -f2 | tr -d '[[:space:]]')
if [ $RESULT == "3000" ]
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

$SRCDIR/omnicore-cli --regtest stop


