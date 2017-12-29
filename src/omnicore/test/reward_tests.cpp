#include "omnicore/omnicore.h"

#include "test/test_bitcoin.h"

#include <stdint.h>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

extern int64_t GetDevOmni(int64_t nTime);

BOOST_FIXTURE_TEST_SUITE(omnicore_reward, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(exodus_reward)
{
    BOOST_CHECK_EQUAL(507030922981, GetDevOmni(1382289200));
    BOOST_CHECK_EQUAL(507097784113, GetDevOmni(1382289794));
    BOOST_CHECK_EQUAL(520429926063, GetDevOmni(1382408393));
    BOOST_CHECK_EQUAL(697540388159, GetDevOmni(1384013953));
    BOOST_CHECK_EQUAL(716842631388, GetDevOmni(1384192405));
    BOOST_CHECK_EQUAL(802547669807, GetDevOmni(1384993319));
    BOOST_CHECK_EQUAL(802489330654, GetDevOmni(1384992769));
    BOOST_CHECK_EQUAL(855805977969, GetDevOmni(1385498211));
    BOOST_CHECK_EQUAL(1192338778645, GetDevOmni(1388824957));
    BOOST_CHECK_EQUAL(1399173013145, GetDevOmni(1390997148));
    BOOST_CHECK_EQUAL(1421087314454, GetDevOmni(1391233485));
    BOOST_CHECK_EQUAL(2202414632241, GetDevOmni(1400578380));
    BOOST_CHECK_EQUAL(2645403602130, GetDevOmni(1406875734));
    BOOST_CHECK_EQUAL(2684375561916, GetDevOmni(1407473800));
    BOOST_CHECK_EQUAL(3055943459392, GetDevOmni(1413608941));
    BOOST_CHECK_EQUAL(3171498752092, GetDevOmni(1415698701));
    BOOST_CHECK_EQUAL(3368727376231, GetDevOmni(1419503239));
    BOOST_CHECK_EQUAL(3556440579708, GetDevOmni(1423445704));
    BOOST_CHECK_EQUAL(3566221821137, GetDevOmni(1423660800));
    BOOST_CHECK_EQUAL(3824406032810, GetDevOmni(1429740313));
    BOOST_CHECK_EQUAL(3899735848397, GetDevOmni(1431678690));
    BOOST_CHECK_EQUAL(3899726604439, GetDevOmni(1431678447));
    BOOST_CHECK_EQUAL(3928686564407, GetDevOmni(1432446165));
    BOOST_CHECK_EQUAL(4020119315049, GetDevOmni(1434958635));
    BOOST_CHECK_EQUAL(4020116412520, GetDevOmni(1434958553));
    BOOST_CHECK_EQUAL(4069615916388, GetDevOmni(1436378901));
    BOOST_CHECK_EQUAL(4086969586261, GetDevOmni(1436887530));
    BOOST_CHECK_EQUAL(4111853481632, GetDevOmni(1437626928));
    BOOST_CHECK_EQUAL(4243857803360, GetDevOmni(1441763694));
    BOOST_CHECK_EQUAL(4282633068282, GetDevOmni(1443053864));
    BOOST_CHECK_EQUAL(4502418971303, GetDevOmni(1451150528));
    BOOST_CHECK_EQUAL(4503741929381, GetDevOmni(1451203898));
    BOOST_CHECK_EQUAL(4511956596124, GetDevOmni(1451536697));
    BOOST_CHECK_EQUAL(4521189218488, GetDevOmni(1451913663));
    BOOST_CHECK_EQUAL(4521480990078, GetDevOmni(1451925627));
}


BOOST_AUTO_TEST_SUITE_END()
