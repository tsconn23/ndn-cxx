/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2019 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "ndn-cxx/interest.hpp"
#include "ndn-cxx/data.hpp"
#include "ndn-cxx/security/digest-sha256.hpp"
#include "ndn-cxx/security/signature-sha256-with-rsa.hpp"

#include "tests/boost-test.hpp"
#include "tests/make-interest-data.hpp"

namespace ndn {
namespace tests {

BOOST_AUTO_TEST_SUITE(TestInterest)

// ---- constructor, encode, decode ----

BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
  Interest i;
  BOOST_CHECK(!i.hasWire());
  BOOST_CHECK_EQUAL(i.getName(), "/");
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), true);
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), false);
  BOOST_CHECK(i.getForwardingHint().empty());
  BOOST_CHECK(!i.hasNonce());
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK(!i.hasSelectors());
  BOOST_CHECK(!i.hasApplicationParameters());
  BOOST_CHECK(i.getApplicationParameters().empty());
}

BOOST_AUTO_TEST_CASE(DecodeNotInterest)
{
  BOOST_CHECK_THROW(Interest("4202CAFE"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(EncodeDecode02Basic)
{
  const uint8_t WIRE[] = {
    0x05, 0x1c, // Interest
          0x07, 0x14, // Name
                0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, // GenericNameComponent
                0x08, 0x03, 0x6e, 0x64, 0x6e, // GenericNameComponent
                0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, // GenericNameComponent
          0x0a, 0x04, // Nonce
                0x01, 0x00, 0x00, 0x00
  };

  Interest i1("/local/ndn/prefix");
  i1.setCanBePrefix(true);
  i1.setNonce(1);
  Block wire1 = i1.wireEncode();
  BOOST_CHECK_EQUAL_COLLECTIONS(wire1.begin(), wire1.end(), WIRE, WIRE + sizeof(WIRE));

  Interest i2(wire1);
  BOOST_CHECK_EQUAL(i2.getName(), "/local/ndn/prefix");
  BOOST_CHECK(i2.getSelectors().empty());
  BOOST_CHECK_EQUAL(i2.getNonce(), 1);
  BOOST_CHECK_EQUAL(i2.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);

  BOOST_CHECK_EQUAL(i1, i2);
}

BOOST_AUTO_TEST_CASE(EncodeDecode02Full)
{
  const uint8_t WIRE[] = {
    0x05, 0x31, // Interest
          0x07, 0x14, // Name
                0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, // GenericNameComponent
                0x08, 0x03, 0x6e, 0x64, 0x6e, // GenericNameComponent
                0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, // GenericNameComponent
          0x09, 0x03, // Selectors
                0x0d, 0x01, 0x01,  // MinSuffixComponents
          0x0a, 0x04, // Nonce
                0x01, 0x00, 0x00, 0x00,
          0x0c, 0x02, // InterestLifetime
                0x03, 0xe8,
          0x1e, 0x0a, // ForwardingHint
                0x1f, 0x08, // Delegation
                      0x1e, 0x01, 0x01, // Preference=1
                      0x07, 0x03, 0x08, 0x01, 0x41 // Name=/A
  };

  Interest i1;
  i1.setName("/local/ndn/prefix");
  i1.setCanBePrefix(true);
  i1.setMinSuffixComponents(1);
  i1.setNonce(1);
  i1.setInterestLifetime(1000_ms);
  i1.setForwardingHint({{1, "/A"}});
  Block wire1 = i1.wireEncode();
  BOOST_CHECK_EQUAL_COLLECTIONS(wire1.begin(), wire1.end(), WIRE, WIRE + sizeof(WIRE));

  Interest i2(wire1);
  BOOST_CHECK_EQUAL(i2.getName(), "/local/ndn/prefix");
  BOOST_CHECK_EQUAL(i2.getMinSuffixComponents(), 1);
  BOOST_CHECK_EQUAL(i2.getNonce(), 1);
  BOOST_CHECK_EQUAL(i2.getInterestLifetime(), 1000_ms);
  BOOST_CHECK_EQUAL(i2.getForwardingHint(), DelegationList({{1, "/A"}}));

  BOOST_CHECK_EQUAL(i1, i2);
}

BOOST_AUTO_TEST_CASE(EncodeDecode03Basic)
{
  const uint8_t WIRE[] = {
    0x05, 0x22, // Interest
          0x07, 0x14, // Name
                0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, // GenericNameComponent
                0x08, 0x03, 0x6e, 0x64, 0x6e, // GenericNameComponent
                0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, // GenericNameComponent
          0x0a, 0x04, // Nonce
                0x01, 0x00, 0x00, 0x00,
          0x24, 0x04, // ApplicationParameters
                0xc0, 0xc1, 0xc2, 0xc3};

  Interest i1;
  i1.setName("/local/ndn/prefix");
  i1.setCanBePrefix(false);
  i1.setNonce(1);
  i1.setApplicationParameters("2404C0C1C2C3"_block);
  Block wire1 = i1.wireEncode();
  BOOST_CHECK_EQUAL_COLLECTIONS(wire1.begin(), wire1.end(), WIRE, WIRE + sizeof(WIRE));

  Interest i2(wire1);
  BOOST_CHECK_EQUAL(i2.getName(), "/local/ndn/prefix");
  BOOST_CHECK_EQUAL(i2.getCanBePrefix(), false);
  BOOST_CHECK_EQUAL(i2.getMustBeFresh(), false);
  BOOST_CHECK(i2.getForwardingHint().empty());
  BOOST_CHECK_EQUAL(i2.getNonce(), 1);
  BOOST_CHECK_EQUAL(i2.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK(i2.hasApplicationParameters());
  BOOST_CHECK_EQUAL(i2.getApplicationParameters(), "2404C0C1C2C3"_block);
  BOOST_CHECK(i2.getPublisherPublicKeyLocator().empty());
}

BOOST_AUTO_TEST_CASE(EncodeDecode03Full)
{
  const uint8_t WIRE[] = {
    0x05, 0x37, // Interest
          0x07, 0x14, // Name
                0x08, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, // GenericNameComponent
                0x08, 0x03, 0x6e, 0x64, 0x6e, // GenericNameComponent
                0x08, 0x06, 0x70, 0x72, 0x65, 0x66, 0x69, 0x78, // GenericNameComponent
          0x21, 0x00, // CanBePrefix
          0x12, 0x00, // MustBeFresh
          0x1e, 0x0b, // ForwardingHint
                0x1f, 0x09, // Delegation List
                      0x1e, 0x02,
                            0x3e, 0x15,
                      0x07, 0x03,
                            0x08, 0x01, 0x48,
          0x0a, 0x04, // Nonce
                0x4a, 0xcb, 0x1e, 0x4c,
          0x0c, 0x02, // Interest Lifetime
                0x76, 0xa1,
          0x24, 0x04, // ApplicationParameters
                0xc0, 0xc1, 0xc2, 0xc3};
  Interest i1;
  i1.setName("/local/ndn/prefix");
  i1.setMustBeFresh(true);
  i1.setCanBePrefix(true);
  i1.setForwardingHint(DelegationList({{15893, "/H"}}));
  i1.setNonce(0x4c1ecb4a);
  i1.setInterestLifetime(30369_ms);
  i1.setApplicationParameters("2404C0C1C2C3"_block);
  i1.setMinSuffixComponents(1); // v0.2-only elements will not be encoded
  i1.setExclude(Exclude().excludeAfter(name::Component("J"))); // v0.2-only elements will not be encoded
  Block wire1 = i1.wireEncode();
  BOOST_CHECK_EQUAL_COLLECTIONS(wire1.begin(), wire1.end(), WIRE, WIRE + sizeof(WIRE));

  Interest i2(wire1);
  BOOST_CHECK_EQUAL(i2.getName(), "/local/ndn/prefix");
  BOOST_CHECK_EQUAL(i2.getCanBePrefix(), true);
  BOOST_CHECK_EQUAL(i2.getMustBeFresh(), true);
  BOOST_CHECK_EQUAL(i2.getForwardingHint(), DelegationList({{15893, "/H"}}));
  BOOST_CHECK(i2.hasNonce());
  BOOST_CHECK_EQUAL(i2.getNonce(), 0x4c1ecb4a);
  BOOST_CHECK_EQUAL(i2.getInterestLifetime(), 30369_ms);
  BOOST_CHECK_EQUAL(i2.getApplicationParameters(), "2404C0C1C2C3"_block);
  BOOST_CHECK_EQUAL(i2.getMinSuffixComponents(), -1); // Default because minSuffixComponents was not encoded
  BOOST_CHECK(i2.getExclude().empty()); // Exclude was not encoded
}

class Decode03Fixture
{
protected:
  Decode03Fixture()
  {
    // initialize all elements to non-empty, to verify wireDecode clears them
    i.setName("/A");
    i.setForwardingHint({{10309, "/F"}});
    i.setNonce(0x03d645a8);
    i.setInterestLifetime(18554_ms);
    i.setPublisherPublicKeyLocator(Name("/K"));
    i.setApplicationParameters("2404A0A1A2A3"_block);
  }

protected:
  Interest i;
};

BOOST_FIXTURE_TEST_SUITE(Decode03, Decode03Fixture)

BOOST_AUTO_TEST_CASE(Minimal)
{
  i.wireDecode("0505 0703080149"_block);
  BOOST_CHECK_EQUAL(i.getName(), "/I");
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), false);
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), false);
  BOOST_CHECK(i.getForwardingHint().empty());
  BOOST_CHECK(i.hasNonce()); // a random nonce is generated
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK(i.getPublisherPublicKeyLocator().empty());
  BOOST_CHECK(!i.hasApplicationParameters());

  BOOST_CHECK(!i.hasWire()); // nonce generation resets wire encoding

  // modify then re-encode as v0.2 format
  i.setNonce(0x54657c95);
  BOOST_CHECK_EQUAL(i.wireEncode(), "0510 0703080149 09030E0101 0A04957C6554"_block);
}

BOOST_AUTO_TEST_CASE(Full)
{
  i.wireDecode("0531 0703080149 FC00 2100 FC00 1200 "
               "FC00 1E0B(1F09 1E023E15 0703080148) FC00 0A044ACB1E4C "
               "FC00 0C0276A1 FC00 2201D6 FC00"_block);
  BOOST_CHECK_EQUAL(i.getName(), "/I");
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), true);
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), true);
  BOOST_CHECK_EQUAL(i.getForwardingHint(), DelegationList({{15893, "/H"}}));
  BOOST_CHECK(i.hasNonce());
  BOOST_CHECK_EQUAL(i.getNonce(), 0x4c1ecb4a);
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), 30369_ms);
  // HopLimit=214 is not stored

  // encode without modification: retain original wire encoding
  BOOST_CHECK_EQUAL(i.wireEncode().value_size(), 49);

  // modify then re-encode as v0.2 format
  i.setName("/J");
  BOOST_CHECK_EQUAL(i.wireEncode(),
    "0520 070308014A 09021200 0A044ACB1E4C 0C0276A1 1E0B(1F09 1E023E15 0703080148)"_block);
}

BOOST_AUTO_TEST_CASE(CriticalElementOutOfOrder)
{
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 2100 0703080149 1200 1E0B(1F09 1E023E15 0703080148) "
    "0A044ACB1E4C 0C0276A1 2201D6 2404C0C1C2C3"_block),
    tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 0703080149 1200 2100 1E0B(1F09 1E023E15 0703080148) "
    "0A044ACB1E4C 0C0276A1 2201D6 2404C0C1C2C3"_block),
    tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 0703080149 2100 1E0B(1F09 1E023E15 0703080148) 1200 "
    "0A044ACB1E4C 0C0276A1 2201D6 2404C0C1C2C3"_block),
    tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 0703080149 2100 1200 0A044ACB1E4C "
    "1E0B(1F09 1E023E15 0703080148) 0C0276A1 2201D6 2404C0C1C2C3"_block),
    tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 0703080149 2100 1200 1E0B(1F09 1E023E15 0703080148) "
    "0C0276A1 0A044ACB1E4C 2201D6 2404C0C1C2C3"_block),
    tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode(
    "0529 0703080149 2100 1200 1E0B(1F09 1E023E15 0703080148) "
    "0A044ACB1E4C 2201D6 0C0276A1 2404C0C1C2C3"_block),
    tlv::Error);
}

BOOST_AUTO_TEST_CASE(NonCriticalElementOutOfOrder)
{
  // HopLimit
  i.wireDecode("0514 0703080149 2201D6 2200 2404C0C1C2C3 22020101"_block);
  BOOST_CHECK_EQUAL(i.getName(), "/I");
  // HopLimit=214 is not stored
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2404C0C1C2C3"_block);

  // ApplicationParameters
  i.wireDecode("051F 0703080149 2100 1200 0A044ACB1E4C 0C0276A1 2201D6 2404C0C1C2C3 2401EE"_block);
  BOOST_CHECK_EQUAL(i.getName(), "/I");
  BOOST_CHECK_EQUAL(i.hasApplicationParameters(), true);
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2404C0C1C2C3"_block);
}

BOOST_AUTO_TEST_CASE(NameMissing)
{
  BOOST_CHECK_THROW(i.wireDecode("0500"_block), tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode("0502 1200"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(NameEmpty)
{
  BOOST_CHECK_THROW(i.wireDecode("0502 0700"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(BadCanBePrefix)
{
  BOOST_CHECK_THROW(i.wireDecode("0508 0703080149 210102"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(BadMustBeFresh)
{
  BOOST_CHECK_THROW(i.wireDecode("0508 0703080149 120102"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(BadNonce)
{
  BOOST_CHECK_THROW(i.wireDecode("0507 0703080149 0A00"_block), tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode("050A 0703080149 0A0304C263"_block), tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode("050C 0703080149 0A05EFA420B262"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(BadHopLimit)
{
  BOOST_CHECK_THROW(i.wireDecode("0507 0703080149 2200"_block), tlv::Error);
  BOOST_CHECK_THROW(i.wireDecode("0509 0703080149 22021356"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(UnrecognizedNonCriticalElementBeforeName)
{
  BOOST_CHECK_THROW(i.wireDecode("0507 FC00 0703080149"_block), tlv::Error);
}

BOOST_AUTO_TEST_CASE(UnrecognizedCriticalElement)
{
  BOOST_CHECK_THROW(i.wireDecode("0507 0703080149 FB00"_block), tlv::Error);
}

BOOST_AUTO_TEST_SUITE_END() // Decode03

// ---- matching ----

BOOST_AUTO_TEST_CASE(MatchesData)
{
  auto interest = makeInterest("/A");

  auto data = makeData("/A");
  BOOST_CHECK_EQUAL(interest->matchesData(*data), true);

  data->setName("/A/D");
  BOOST_CHECK_EQUAL(interest->matchesData(*data), false); // violates CanBePrefix

  interest->setCanBePrefix(true);
  BOOST_CHECK_EQUAL(interest->matchesData(*data), true);

  interest->setMustBeFresh(true);
  BOOST_CHECK_EQUAL(interest->matchesData(*data), false); // violates MustBeFresh

  data->setFreshnessPeriod(1_s);
  BOOST_CHECK_EQUAL(interest->matchesData(*data), true);

  data->setName("/H/I");
  BOOST_CHECK_EQUAL(interest->matchesData(*data), false); // Name does not match

  data->wireEncode();
  interest = makeInterest(data->getFullName());
  BOOST_CHECK_EQUAL(interest->matchesData(*data), true);

  setNameComponent(*interest, -1, Name("/sha256digest=000000000000000000000000"
                                       "0000000000000000000000000000000000000000").at(0));
  BOOST_CHECK_EQUAL(interest->matchesData(*data), false); // violates implicit digest
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(MatchesInterest, 1)
BOOST_AUTO_TEST_CASE(MatchesInterest)
{
  Interest interest("/A");
  interest.setCanBePrefix(true)
          .setMustBeFresh(true)
          .setForwardingHint({{1, "/H"}})
          .setNonce(2228)
          .setInterestLifetime(5_s);

  Interest other;
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), false);

  other.setName(interest.getName());
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), false);

  other.setCanBePrefix(interest.getCanBePrefix());
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), false);

  other.setMustBeFresh(interest.getMustBeFresh());
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), false); // will match until #3162 implemented

  other.setForwardingHint(interest.getForwardingHint());
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), true);

  other.setNonce(9336);
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), true);

  other.setInterestLifetime(3_s);
  BOOST_CHECK_EQUAL(interest.matchesInterest(other), true);
}

// ---- field accessors ----

BOOST_AUTO_TEST_CASE(CanBePrefix)
{
  Interest i;
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), true);
  i.setCanBePrefix(false);
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), false);
  BOOST_CHECK_EQUAL(i.getSelectors().getMaxSuffixComponents(), 1);
  i.setCanBePrefix(true);
  BOOST_CHECK_EQUAL(i.getCanBePrefix(), true);
  BOOST_CHECK_EQUAL(i.getSelectors().getMaxSuffixComponents(), -1);
}

BOOST_AUTO_TEST_CASE(MustBeFresh)
{
  Interest i;
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), false);
  i.setMustBeFresh(true);
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), true);
  BOOST_CHECK_EQUAL(i.getSelectors().getMustBeFresh(), true);
  i.setMustBeFresh(false);
  BOOST_CHECK_EQUAL(i.getMustBeFresh(), false);
  BOOST_CHECK_EQUAL(i.getSelectors().getMustBeFresh(), false);
}

BOOST_AUTO_TEST_CASE(ModifyForwardingHint)
{
  Interest i;
  i.setCanBePrefix(false);
  i.setForwardingHint({{1, "/A"}});
  i.wireEncode();
  BOOST_CHECK(i.hasWire());

  i.modifyForwardingHint([] (DelegationList& fh) { fh.insert(2, "/B"); });
  BOOST_CHECK(!i.hasWire());
  BOOST_CHECK_EQUAL(i.getForwardingHint(), DelegationList({{1, "/A"}, {2, "/B"}}));
}

BOOST_AUTO_TEST_CASE(GetNonce)
{
  unique_ptr<Interest> i1, i2;

  // getNonce automatically assigns a random Nonce.
  // It's possible to assign the same Nonce to two Interest, but it's unlikely to get 100 pairs of
  // same Nonces in a row.
  int nIterations = 0;
  uint32_t nonce1 = 0, nonce2 = 0;
  do {
    i1 = make_unique<Interest>();
    nonce1 = i1->getNonce();
    i2 = make_unique<Interest>();
    nonce2 = i2->getNonce();
  }
  while (nonce1 == nonce2 && ++nIterations < 100);
  BOOST_CHECK_NE(nonce1, nonce2);
  BOOST_CHECK(i1->hasNonce());
  BOOST_CHECK(i2->hasNonce());

  // Once a Nonce is assigned, it should not change.
  BOOST_CHECK_EQUAL(i1->getNonce(), nonce1);
}

BOOST_AUTO_TEST_CASE(SetNonce)
{
  Interest i1("/A");
  i1.setCanBePrefix(false);
  i1.setNonce(1);
  i1.wireEncode();
  BOOST_CHECK_EQUAL(i1.getNonce(), 1);

  Interest i2(i1);
  BOOST_CHECK_EQUAL(i2.getNonce(), 1);

  i2.setNonce(2);
  BOOST_CHECK_EQUAL(i2.getNonce(), 2);
  BOOST_CHECK_EQUAL(i1.getNonce(), 1); // should not affect i1 Nonce (Bug #4168)
}

BOOST_AUTO_TEST_CASE(RefreshNonce)
{
  Interest i;
  BOOST_CHECK(!i.hasNonce());
  i.refreshNonce();
  BOOST_CHECK(!i.hasNonce());

  i.setNonce(1);
  BOOST_CHECK(i.hasNonce());
  i.refreshNonce();
  BOOST_CHECK(i.hasNonce());
  BOOST_CHECK_NE(i.getNonce(), 1);
}

BOOST_AUTO_TEST_CASE(SetInterestLifetime)
{
  BOOST_CHECK_THROW(Interest("/A", -1_ms), std::invalid_argument);
  BOOST_CHECK_NO_THROW(Interest("/A", 0_ms));

  Interest i("/local/ndn/prefix");
  i.setNonce(1);
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  BOOST_CHECK_THROW(i.setInterestLifetime(-1_ms), std::invalid_argument);
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), DEFAULT_INTEREST_LIFETIME);
  i.setInterestLifetime(0_ms);
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), 0_ms);
  i.setInterestLifetime(1_ms);
  BOOST_CHECK_EQUAL(i.getInterestLifetime(), 1_ms);
}

BOOST_AUTO_TEST_CASE(SetApplicationParameters)
{
  const uint8_t PARAMETERS1[] = {0xc1};
  const uint8_t PARAMETERS2[] = {0xc2};

  Interest i;
  BOOST_CHECK(!i.hasApplicationParameters());
  i.setApplicationParameters("2400"_block);
  BOOST_CHECK(i.hasApplicationParameters());
  i.unsetApplicationParameters();
  BOOST_CHECK(!i.hasApplicationParameters());

  // Block overload
  i.setApplicationParameters(Block{});
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2400"_block);
  i.setApplicationParameters("2401C0"_block);
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2401C0"_block);
  i.setApplicationParameters("8001C1"_block);
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "24038001C1"_block);

  // raw buffer+size overload
  i.setApplicationParameters(PARAMETERS1, sizeof(PARAMETERS1));
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2401C1"_block);
  i.setApplicationParameters(nullptr, 0);
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2400"_block);
  BOOST_CHECK_THROW(i.setApplicationParameters(nullptr, 42), std::invalid_argument);

  // ConstBufferPtr overload
  i.setApplicationParameters(make_shared<Buffer>(PARAMETERS2, sizeof(PARAMETERS2)));
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2401C2"_block);
  i.setApplicationParameters(make_shared<Buffer>());
  BOOST_CHECK_EQUAL(i.getApplicationParameters(), "2400"_block);
  BOOST_CHECK_THROW(i.setApplicationParameters(nullptr), std::invalid_argument);
}

// ---- operators ----

BOOST_AUTO_TEST_CASE(Equality)
{
  Interest a;
  Interest b;

  // if nonce is not set, it would be set to a random value
  a.setNonce(1);
  b.setNonce(1);

  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare Name
  a.setName("/A");
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setName("/B");
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setName("/A");
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare Selectors
  a.setChildSelector(1);
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setChildSelector(1);
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare Nonce
  a.setNonce(100);
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setNonce(100);
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare InterestLifetime
  a.setInterestLifetime(10_s);
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setInterestLifetime(10_s);
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare ForwardingHint
  a.setForwardingHint({{1, "/H"}});
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setForwardingHint({{1, "/H"}});
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);

  // compare ApplicationParameters
  a.setApplicationParameters("2404C0C1C2C3"_block);
  BOOST_CHECK_EQUAL(a == b, false);
  BOOST_CHECK_EQUAL(a != b, true);

  b.setApplicationParameters("2404C0C1C2C3"_block);
  BOOST_CHECK_EQUAL(a == b, true);
  BOOST_CHECK_EQUAL(a != b, false);
}

BOOST_AUTO_TEST_SUITE_END() // TestInterest

} // namespace tests
} // namespace ndn