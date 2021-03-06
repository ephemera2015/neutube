#ifndef ZSTRINGTEST_H
#define ZSTRINGTEST_H

#include "ztestheader.h"
#include "neutubeconfig.h"
#include "zstring.h"
#include "biocytin/zbiocytinfilenameparser.h"

#ifdef _USE_GTEST_

TEST(ZString, extractWord) {
  ZString str = "\"This\" is a test";
  EXPECT_EQ("This", str.firstQuotedWord());

  str = "\"This is a test";
  EXPECT_EQ("", str.firstQuotedWord());

  str = "This\" is a test";
  EXPECT_EQ("", str.firstQuotedWord());

  str = "\"This\" \"is\" a test";
  EXPECT_EQ("This", str.firstQuotedWord());

  str = "This \"is\" a test";
  EXPECT_EQ("is", str.firstQuotedWord());

  str = "This \"\"is a test";
  EXPECT_EQ("", str.firstQuotedWord());
}

TEST(ZString, tokenize)
{
  std::string str = "/Users/foo/test/DH070613-1-2.tif";

  std::vector<std::string> strArray = ZString::Tokenize(str, '/');
  ASSERT_EQ(5, (int) strArray.size());
  ASSERT_EQ("", strArray[0]);
  ASSERT_EQ("Users", strArray[1]);
  ASSERT_EQ("foo", strArray[2]);
  ASSERT_EQ("test", strArray[3]);
  ASSERT_EQ("DH070613-1-2.tif", strArray[4]);

  std::vector<std::string> strArray2 = ZString(str).tokenize('/');
  ASSERT_EQ(5, (int) strArray2.size());
  ASSERT_EQ("", strArray2[0]);
  ASSERT_EQ("Users", strArray2[1]);
  ASSERT_EQ("foo", strArray2[2]);
  ASSERT_EQ("test", strArray2[3]);
  ASSERT_EQ("DH070613-1-2.tif", strArray2[4]);

  std::vector<std::string> strArray3 = ZString::ToWordArray(str, "/");
  ASSERT_EQ(4, (int) strArray3.size());
  ASSERT_EQ("Users", strArray3[0]);
  ASSERT_EQ("foo", strArray3[1]);
  ASSERT_EQ("test", strArray3[2]);
  ASSERT_EQ("DH070613-1-2.tif", strArray3[3]);

  str = "http://emdata2.int.janelia.org:7000/api/repo/752dbb063ca346f3a3e081eacebd253b/info";
  std::vector<std::string> strArray4 = ZString::ToWordArray(str, "/:");
  ASSERT_EQ(7, (int) strArray4.size());
  ASSERT_EQ("http", strArray4[0]);
  ASSERT_EQ("emdata2.int.janelia.org", strArray4[1]);
  ASSERT_EQ("7000", strArray4[2]);
  ASSERT_EQ("api", strArray4[3]);
  ASSERT_EQ("repo", strArray4[4]);
  ASSERT_EQ("752dbb063ca346f3a3e081eacebd253b", strArray4[5]);
  ASSERT_EQ("info", strArray4[6]);

  std::vector<std::string> strArray5 = ZString(str).toWordArray(":/");
  ASSERT_EQ(7, (int) strArray5.size());
  ASSERT_EQ("http", strArray5[0]);
  ASSERT_EQ("emdata2.int.janelia.org", strArray5[1]);
  ASSERT_EQ("7000", strArray5[2]);
  ASSERT_EQ("api", strArray5[3]);
  ASSERT_EQ("repo", strArray5[4]);
  ASSERT_EQ("752dbb063ca346f3a3e081eacebd253b", strArray5[5]);
  ASSERT_EQ("info", strArray5[6]);
}

TEST(ZString, ParseFileName)
{
  std::string str = "/Users/foo/test/DH070613-1-2.tif";

  EXPECT_EQ("DH070613-1-2.tif", ZString::getBaseName(str));
  EXPECT_EQ("DH070613-1-2",
            ZString::removeFileExt(ZString::getBaseName(str)));
}

TEST(ZBiocytinFileNameParser, Basic)
{
  std::string str = "/Users/foo/test/DH070613-1-2.tif";

  EXPECT_EQ("DH070613-1-2", ZBiocytinFileNameParser::getCoreName(str));

  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.Edit.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.edit.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.proj.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.roi.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.ROI.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.mask.tif"));
  EXPECT_EQ("DH070613-1-2",
            ZBiocytinFileNameParser::getCoreName("DH070613-1-2.Mask.tif"));


  EXPECT_EQ(ZBiocytinFileNameParser::ORIGINAL,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::EDIT,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.Edit.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::EDIT,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.edit.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::PROJECTION,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.proj.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::ROI,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.roi.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::ROI,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.ROI.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::MASK,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.mask.tif"));
  EXPECT_EQ(ZBiocytinFileNameParser::MASK,
            ZBiocytinFileNameParser::getRole("DH070613-1-2.Mask.tif"));
}

#endif

#endif // ZSTRINGTEST_H
