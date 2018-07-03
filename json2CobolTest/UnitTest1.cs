using System;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;

namespace json2CobolTest
{
    struct json_element
    {
        readonly string name;
        readonly string type;
        readonly string length;
        readonly string arrayCountLen;
        readonly string arrayMaxLen;
        readonly string parentIndex;
        public json_element(string _name,string _type, string _length, string _arrayCountLen, string _arrayMaxLen, string _parentIndex) { name = _name; type = _type; length = _length; arrayCountLen = _arrayCountLen; arrayMaxLen = _arrayMaxLen; parentIndex = _parentIndex; }
        public json_element(string _name, string _type, string _length, string _parentIndex) { name = _name; type = _type; length = _length; arrayCountLen = ""; arrayMaxLen = ""; parentIndex = _parentIndex; }
        public override string ToString() { return name + "\0" + type + "\0" + length + "\0" + (arrayCountLen==""? "": arrayCountLen + "\0") + (arrayMaxLen == "" ? "" : arrayMaxLen + "\0")  + (parentIndex == "" ? "-1\0" : parentIndex + "\0" );  }
    };

    [TestClass]
    public class UnitTest1
    {
        [DllImport("json2CobolCopyBook.dll")]
        private static extern int parse(string inputJson, int count, string structure, byte[] output);

        [DllImport("json2CobolCopyBook.dll")]
        private static extern int print_line(string str);

        public bool compare(string expected, byte[] actual, int start, int len)
        {
            for (int i = start; i < len; i++)
            {
                if (expected[i] != actual[i])
                {
                    Console.WriteLine("Expected\t:" + expected );
                    Console.WriteLine("Actual  \t:" + Encoding.Default.GetString(actual) + "\n");
                    Console.WriteLine("differ at " + i + " found char " + actual[i].ToString() + " but wanted " + expected[i]);
                    return false;
                };
            }
            return true;
        }

        [TestMethod]
        public void TestCanHookIntoDLL()
        {
            var x = print_line("Hello, PInvoke!");
        }
        [TestMethod]
        public void TestSingleString()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;


            string s = "";
            int result = 0;
            result = 0;
            s = new json_element("hi", "s", "5", "0").ToString();
            result = parse("{\"hi\":\"a\"}", 1, s, output);
            Assert.AreEqual(result, 2);
            const string Expected = "a ";
            Assert.IsTrue(compare(Expected, output, 0, Expected.Length));
        }

        [TestMethod]
        public void TestSingleNumber()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;
        
            int result = parse("{\"hi\":1}", 1, "hi\0n\000005\00\0", output);
            Assert.AreEqual(result, 2);
            const string Expected = "00001 ";
            Assert.IsTrue(compare(Expected, output, 0, Expected.Length));
        }

        [TestMethod]
        public void TestSingleBool()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            int result = parse("{\"hi\":true}", 1, "hi\0b\000005\00\0", output);
            Assert.AreEqual(result, 2);
            const string Expected = "1\0\0\0\0 ";
            Assert.IsTrue(compare(Expected, output, 0, Expected.Length));
        }
        [TestMethod]
        public void TestSimpleArray()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("arr", "ab", "3", "2", "5", "0").ToString() +
                       new json_element("", "n", "3", "1");

            int result = parse("{\"arr\":[1,2,3]}", 2, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("03001002003 ", output, 0, 12));
        }

        [TestMethod]
        public void TestSimpleArrayFollowdBySomething()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("arr", "ab", "3", "2", "5", "0").ToString() +
                       new json_element("", "n", "3", "1") +
                       new json_element("bye", "s", "8", "0");

            int result = parse("{\"arr\":[1,2,3],\"bye\":\"the end\"}", 3, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("03001002003the end ", output, 0, 12));
        }

        [TestMethod]
        public void TestArrayofArrays()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s =  new json_element("arr", "ab", "3", "2", "5", "0").ToString() +
                        new json_element("", "ab", "3", "2", "5", "1").ToString() +
                        new json_element("", "n", "3", "2");

            int result = parse("{\"arr\":[[1,2,3],[4,5,6],[7],[8,9]],\"bye\":\"the end\"}", 3, s, output);
            Assert.AreEqual(result, 2);

            const string Expected = "0403001002003030040050060100702008009 ";
            Assert.IsTrue(compare(Expected, output, 0, Expected.Length));
        }

        //[TestMethod]
        public void TestArrayofObjectsWithArrays()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("arr", "ab", "3", "2", "5", "0").ToString() +
                       new json_element("", "o", "0", "1") +
                       new json_element("", "ab", "3", "2", "5", "2").ToString() +
                       new json_element("", "n", "3", "3");

            int result = parse("{\"arr\":[{[1,2,3],[4,5,6]},{[7],[8,9]}],\"bye\":\"the end\"}", 4, s, output);
            Assert.AreEqual(result, 2);

            const string Expected = "0403001002003030040050060100702008009 ";
            Assert.IsTrue(compare(Expected, output, 0, Expected.Length));
        }


        [TestMethod]
        public void TestSimpleArrayExceedsCount()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("arr", "ab", "3", "2", "5", "0").ToString() +
                       new json_element("", "n", "3", "1");

            //Assert.AreEqual("arr\0ab\03\02\05\0\0",s);

            int result = parse("{\"arr\":[1,2,3,4,5,6,7,8]}", 2, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("05001002003004005 ", output, 0, 18));
        }

        [TestMethod]
        public void TestNoProperties()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("arr", "ab", "3", "2", "5", "").ToString();

            int result = parse("{\"p123\":true, \"p456\":\"abcdefg\", \"p789\":123454321 }", 1, "hi\0b\000005\00\0", output);
            Assert.AreEqual(result, -10);
        }
        [TestMethod]
        public void TestTwoProperties()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("p456", "s", "10", "0").ToString() +
                       new json_element("p789", "n", "8", "0");

            int result = parse("{\"p123\":true, \"p456\":\"abcdefg\", \"p789\":12344321 }", 2, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("abcdefg   12344321 ", output, 0, 19));

        }
        [TestMethod]
        public void TestTwoObjProperties()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("", "o", "0", "0").ToString() +
                       new json_element("p456", "s", "10", "1").ToString() +
                       new json_element("p789", "n", "8", "1");

            int result = parse("{{\"p123\":true, \"p456\":\"abcdefg\", \"p789\":12344321 }}", 3, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("abcdefg   12344321 ", output, 0, 19));

        }

        [TestMethod]
        public void TestObjWithNameandtwoPropertiesx()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("a", "o", "0", "0").ToString() +
                       new json_element("p456", "s", "10", "1").ToString() +
                       new json_element("p789", "n", "8", "1");

            int result = parse("{\"a\":{\"p123\":true, \"p456\":\"abcdefg\", \"p789\":12344321 }}", 3, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("abcdefg   12344321 ", output, 0, 19));

        }


        [TestMethod]
        public void TestObjWithNameandtwoPropertiesFollowedBySomething()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string s = new json_element("a", "o", "0", "0").ToString() +
                       new json_element("p2", "s", "10", "1").ToString() +
                       new json_element("p3", "n", "8", "1");

            int result = parse("{\"a\":{\"p1\":true, \"p2\":\"one\", \"p3\":22},\"b\":33}", 3, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("one       00000022 ", output, 0, 19));

        }


        [TestMethod]
        public void TestArrayOfOneObject()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;


            string s = new json_element("hi", "ab", "3", "2", "5", "0").ToString() +
                new json_element("", "o", "0", "1") +
                new json_element("a", "s", "3", "2") +
                new json_element("b", "n", "3", "2");

            int result = parse("{\"hi\":[{\"a\":\"x\", \"b\":1}]}", 4, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("01x  001", output, 0, 08));
        }
        [TestMethod]
        public void TestArrayOfThreeObjectsFollwedbySomething()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;


            string s = new json_element("hi", "ab", "3", "2", "5", "0").ToString() +
                new json_element("", "o", "0", "1") +
                new json_element("a", "s", "3", "2") +
                new json_element("b", "n", "3", "2");

            int result = parse("{\"hi\":[{\"a\":\"w\", \"b\":1},{\"a\":\"x\", \"b\":2}, {\"c\":\"y\", \"b\":3}, {\"a\":\"z\", \"b\":4}], \"bye\":\"xxx\"}", 4, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("04w  001x  002003z  004", output, 0, 23));
        }

        [TestMethod]
        public void TestArrayOfThreeObjectsFollwedbySomething2()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;


            string s = new json_element("hi", "ab", "3", "2", "5", "0").ToString() +
                new json_element("", "o", "0", "1") +
                new json_element("a", "s", "3", "2") +
                new json_element("b", "n", "3", "2");

            int result = parse("{\"hi\":[{\"a\":\"w\", \"b\":1},{\"a\":\"x\", \"b\":2}, {\"c\":\"y\", \"d\":3}, {\"a\":\"z\", \"b\":4}], \"bye\":\"xxx\"}", 4, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("04w  001x  002z  004", output, 0, 20));
        }


        [TestMethod]
        public void TestArrayOfThreeObjectsFollwedbySomething3()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;


            string s = new json_element("hi", "ab", "3", "2", "5", "0").ToString() +
                new json_element("", "o", "0", "1") +
                new json_element("a", "s", "3", "2") +
                new json_element("b", "n", "3", "2") +
                new json_element("bye", "s", "10", "0");

            int result = parse("{\"hi\":[{\"a\":\"w\", \"b\":1},{\"a\":\"x\", \"b\":2}, {\"c\":\"y\", \"d\":3}, {\"a\":\"z\", \"b\":4}], \"bye\":\"xxx\"}", 4, s, output);
            Assert.AreEqual(result, 2);
            Assert.IsTrue(compare("04w  001x  002z  004", output, 0, 20));
        }

        [TestMethod]
        public void TestMoreComplexJSON()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string tstJson = "{\n" +
                              "\"array\": [ \n" + 
                              "1, \n" +
                              "123, \n" +
                              "3\n" +
                              " ]," +
                              "\"boolean\": true,\n" +
                              "\"null\": NULL, \n"+
                              "\"number\": 123,\n" +
                              "\"object\": {\n" +
                                "\"a\": \"b\",\n" +
                                "\"c\": \"d\",\n" +
                                "\"e\": \"f\"\n" +
                              "},\n"+
                              "\"string\": \"Hello World\"\n"+
                            "}";

            string s =  new json_element("array", "aa", "3", "2", "5", "0").ToString() +
                        new json_element("", "n", "3", "1") +
                        new json_element("object", "o", "0", "0") +
                        new json_element("c", "s", "4", "3");  

            var result = parse(tstJson, 4,s,  output);
            Assert.AreEqual(2, result);
            Assert.IsTrue(compare("001123003      03d ", output, 0, 19));

            Console.WriteLine("Y = {0}", result);
        }

        [TestMethod]
        public void ZZPhew()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string tstJson = "{" +
                                "\"a1\":1," +
                                "\"x\":{" +
                                    "\"b1\":1.1," +
                                    "{" +
                                        "\"c1\":1.1.1," +
                                        "{" +
                                            "\"d1\":1.1.1.1," +
                                            "\"e1\":1.1.1.2" +
                                        "}," +
                                        "\"f1\":1.1.2," +
                                        "\"g1\":1.1.2" +
                                    "}" +
                                "}," +
                                "\"a2\":2," +
                                "{" +
                                    "\"b2\":2.1," +
                                    "{" +
                                        "\"c2\":2.1.1," +
                                        "{" +
                                            "\"d2\":2.1.1.1," +
                                            "\"e2\":2.1.1.2" +
                                        "}," +
                                        "\"f2\":2.1.2," +
                                        "\"g2\":2.1.2" +
                                    "}" +
                                "}," +
                                "\"a3\":3," +
                                "[" +
                                    "{" +
                                        "\"b3\":3.1.1," +
                                        "{" +
                                            "\"c3\":3.1.1.1," +
                                            "{" +
                                                "\"d3\":3.1.1.1.1," +
                                                "\"e3\":3.1.1.1.2" +
                                            "}," +
                                            "\"f3\":3.1.1.2," +
                                            "\"g3\":3.1.1.3" +
                                        "}" +
                                    "}," +
                                    "4," +
                                    "{" +
                                        "\"b4\":4.1.1," +
                                        "{" +
                                            "\"c4\":4.1.1.1," +
                                            "{" +
                                                "\"d4\":4.1.1.1.1," +
                                                "\"e4\":4.1.1.1.2" +
                                            "}," +
                                            "\"f4\":4.1.1.2," +
                                            "\"g4\":4.1.1.3" +
                                        "}" +
                                    "}" +
                                "], " +
                                "\"a5\":5" +
                            "}";

            string s =
            new json_element("a1", "n", "3", "0").ToString() +      //1
            new json_element("x", "o", "0", "0") +                  //2
            new json_element("b1", "n", "3", "2") +                 //3
            new json_element("", "o", "0", "2") +                   //4
            new json_element("c1", "n", "5", "4");                  //5

            var result = parse(tstJson, 5, s, output);
            Assert.AreEqual(2, result);
            Assert.IsTrue(compare("0011.11.1.1 ", output, 0, 12));
        }

        [TestMethod]
        public void ZZPhew2()
        {
            byte[] output = new byte[100];
            for (int i = 0; i < 100; i++) output[i] = 0x20;
            output[99] = 0x00;

            string tstJson = "{" +
                                "\"a1\":1," +
                                "\"x\":{" +
                                    "\"b1\":1.1," +
                                    "{" +
                                        "\"c1\":1.1.1," +
                                        "{" +
                                            "\"d1\":1.1.1.1," +
                                            "\"e1\":1.1.1.2" +
                                        "}," +
                                        "\"f1\":1.1.1," +
                                        "\"g1\":1.1.2" +
                                    "}" +
                                "}," +
                                "\"a2\":2," +
                                "{" +
                                    "\"b2\":2.1," +
                                    "{" +
                                        "\"c2\":2.1.1," +
                                        "{" +
                                            "\"d2\":2.1.1.1," +
                                            "\"e2\":2.1.1.2" +
                                        "}," +
                                        "\"f2\":2.1.1," +
                                        "\"g2\":2.1.2" +
                                    "}" +
                                "}," +
                                "\"a3\":3," +
                                "[" +
                                    "{" +
                                        "\"b3\":3.1.1," +
                                        "{" +
                                            "\"c3\":3.1.1.1," +
                                            "{" +
                                                "\"d3\":3.1.1.1.1," +
                                                "\"e3\":3.1.1.1.2" +
                                            "}," +
                                            "\"f3\":3.1.1.2," +
                                            "\"g3\":3.1.1.3" +
                                        "}" +
                                    "}," +
                                    "4," +
                                    "{" +
                                        "\"b4\":4.1.1," +
                                        "{" +
                                            "\"c4\":4.1.1.1," +
                                            "{" +
                                                "\"d4\":4.1.1.1.1," +
                                                "\"e4\":4.1.1.1.2" +
                                            "}," +
                                            "\"f4\":4.1.1.2," +
                                            "\"g4\":4.1.1.3" +
                                        "}" +
                                    "}" +
                                "], " +
                                "\"a5\":5" +
                            "}";

            string s =
            new json_element("a1", "n", "3", "0").ToString() +      //1
            new json_element("x", "o", "0", "0") +                  //2
            new json_element("b1", "n", "3", "2") +                 //3
            new json_element("", "o", "0", "2") +                   //4
            new json_element("c1", "n", "8", "4") +                 //5
            new json_element("", "o", "0", "4") +                   //6
            new json_element("d1", "n", "8", "6") +                 //7
            new json_element("e1", "n", "8", "6") +                 //8
            new json_element("f1", "n", "8", "4") +                 //9
            new json_element("g1", "n", "8", "4") +                 //10
            new json_element("a2", "n", "8", "0") +                 //11
            new json_element("", "o", "0", "0") +                   //12
            new json_element("b2", "n", "8", "12") +                //13
            new json_element("", "o", "0", "12") +                  //14
            new json_element("c2", "n", "8", "14") +                //15
            new json_element("", "o", "0", "14") +                  //16
            new json_element("d2", "n", "8", "16") +                //17
            new json_element("e2", "n", "8", "16") +                //18
            new json_element("f2", "n", "8", "14") +                //19
            new json_element("g2", "n", "8", "14") +                //20
            new json_element("a3", "n", "8", "0") +                 //21
            new json_element("", "ab", "3", "2", "5", "0") +        //22 
            new json_element("", "o", "0", "22") +                  //23
            new json_element("b3", "n", "8", "23") +                //24
            new json_element("", "o", "0", "23") +                  //25
            new json_element("", "o", "0", "25") +                  //26
            new json_element("d3", "n", "10", "26");                //27

            int result = parse(tstJson, 27, s, output);
            Assert.AreEqual(2, result);
            Assert.IsTrue(compare(
                "001" +             //a1
                "1.1" +             //b1
                "0001.1.1" +        //c1
                "01.1.1.1" +        //d1
                "01.1.1.2" +        //e1
                "0001.1.1" +        //f1
                "0001.1.2" +        //g1
                "00000002" +        //a2
                "000002.1" +        //b2
                "0002.1.1" +        //c2
                "02.1.1.1" +        //d2
                "02.1.1.2" +        //e2
                "0002.1.1" +        //f2
                "0002.1.2" +        //g2
                "00000003" +        //a3
                "01" +              //array count before  currently return 3, not 1!!
                "0003.1.1" +        //b3
                "03.1.1.1.1",       //d3
                output, 0, 50));

            Console.WriteLine("Y = {0}", result);
        }


        [TestMethod]
        public void ZZPhew3()
        {
            byte[] output = new byte[3000000];
            for (int i = 0; i < 10000; i++) output[i] = 0x20;
            output[99] = 0x00;

            int nI = 50, nJ = 100, nK = 100;

            StringBuilder tstJson = new StringBuilder();
            tstJson.Append("{[");
            StringBuilder results = new StringBuilder();
            results.Append( String.Format("{0:D3}", nI) );

            for (int i = 0; i < nI; i++)
            {
                tstJson.Append( (i == 0 ? "" : ",") + "[");
                results.Append( String.Format("{0:D3}",nJ) );
                for (int j = 0; j < nJ; j++)
                {

                    tstJson.Append( (j == 0 ? "" : ",") + "[");
                    results.Append( String.Format("{0:D3}", nK) );

                    for (int k = 0; k < nK; k++)
                    {
                        tstJson.Append((k == 0 ? "" : ",") + k.ToString());
                        results.Append( String.Format("{0:D5}", k) );
                    }
                    tstJson.Append("]");
                }
                tstJson.Append("]");
            }
            tstJson.Append("]}");

            //name, type, length, count length, max array, parent
            string s =
            new json_element("", "ab", "5", "3", nI.ToString(), "0").ToString() +        //1
            new json_element("", "ab", "5", "3", nJ.ToString(), "1") +                   //2 
            new json_element("", "ab", "5", "3", nK.ToString(), "2") +                   //3 
            new json_element("", "n", "5", "3");                               //4
            var sw = new System.Diagnostics.Stopwatch();
            sw.Start();
            int rc= parse(tstJson.ToString(), 4, s, output);
            sw.Stop();
            var ts = sw.Elapsed;

            var elapsedTime = String.Format("{0:00}:{1:00}:{2:00}.{3:000}", ts.Hours, ts.Minutes, ts.Seconds, ts.Milliseconds);
            Console.WriteLine("RunTime " + elapsedTime);

            Assert.AreEqual(2, rc);

            int l = ((nK * 5 + 5) * nJ + 5) * nI + 5;

            //results = "" + results;

            Assert.IsTrue(compare(results.ToString(), output, 0, results.Length));

        }

    }

}


