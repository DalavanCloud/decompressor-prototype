// -*- C++ -*- */
//
// Copyright 2016 WebAssembly Community Group participants
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Converts textual algorithm into binary file form.

#include "interp/StreamWriter.h"
#include "interp/IntReader.h"
#include "interp/TeeWriter.h"
#include "sexp/FlattenAst.h"
#include "sexp/InflateAst.h"
#include "sexp/TextWriter.h"
#include "sexp-parser/Driver.h"
#include "stream/FileWriter.h"
#include "stream/WriteBackedQueue.h"
#include "utils/ArgsParse.h"

#include <memory>
#include <unistd.h>

using namespace wasm;
using namespace wasm::decode;
using namespace wasm::filt;
using namespace wasm::interp;
using namespace wasm::utils;

namespace {

const char* InputFilename = "-";
const char* OutputFilename = "-";
const char* AlgorithmFilename = "/dev/null";

bool TraceParser = false;
bool TraceLexer = false;

std::shared_ptr<RawStream> getOutput() {
  if (OutputFilename == std::string("-")) {
    return std::make_shared<FdWriter>(STDOUT_FILENO, false);
  }
  return std::make_shared<FileWriter>(OutputFilename);
}

std::shared_ptr<SymbolTable> parseFile(const char* Filename) {
  auto Symtab = std::make_shared<SymbolTable>();
  Driver Parser(Symtab);
  if (TraceParser)
    Parser.setTraceParsing(true);
  if (TraceLexer)
    Parser.setTraceLexing(true);
  if (!Parser.parse(Filename)) {
    fprintf(stderr, "Unable to parse: %s\n", Filename);
    Symtab.reset();
  }
  return Symtab;
}

#define BYTES_PER_LINE_IN_WASM_DEFAULTS 16

void generateHeader(const char* Filename, std::shared_ptr<RawStream> Output) {
  Output->puts(
      "// -*- C++ -*- \n"
      "\n"
      "// *** AUTOMATICALLY GENERATED FILE (DO NOT EDIT)! ***\n"
      "\n"
      "// Copyright 2016 WebAssembly Community Group participants\n"
      "//\n"
      "// Licensed under the Apache License, Version 2.0 (the \"License\");\n"
      "// you may not use this file except in compliance with the License.\n"
      "// You may obtain a copy of the License at\n"
      "//\n"
      "//     http://www.apache.org/licenses/LICENSE-2.0\n"
      "//\n"
      "// Unless required by applicable law or agreed to in writing, software\n"
      "// distributed under the License is distributed on an \"AS IS\" BASIS,\n"
      "// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or "
      "implied.\n"
      "// See the License for the specific language governing permissions and\n"
      "// limitations under the License.\n"
      "\n"
      "// Geneated from: \"");
  Output->puts(InputFilename);
  Output->puts(
      "\"\n"
      "\n");
}

void generateEnterNamespaces(std::vector<const char*>& Namespaces,
                             std::shared_ptr<RawStream> Output) {
  for (const char* Name : Namespaces) {
    Output->puts("namespace ");
    Output->puts(Name);
    Output->puts(
        " {\n"
        "\n");
  }
}

void generateExitNamespaces(std::vector<const char*>& Namespaces,
                            std::shared_ptr<RawStream> Output) {
  for (const char* Name : Namespaces) {
    Output->puts("}  // end of namespace ");
    Output->puts(Name);
    Output->puts(
        "\n"
        "\n");
  }
}

void generateArrayAccessorHeader(const char* FunctionName,
                                 std::shared_ptr<RawStream> Output) {
  Output->puts("const uint8_t* ");
  Output->puts(FunctionName);
  Output->puts("()");
}

void generateArraySizeAccessorHeader(const char* FunctionName,
                                     std::shared_ptr<RawStream> Output) {
  Output->puts("size_t ");
  Output->puts(FunctionName);
  Output->puts("Size()");
}

void generateArrayDecl(const char* InputFilename,
                       std::vector<const char*>& Namespaces,
                       const char* FunctionName,
                       std::shared_ptr<ReadCursor> ReadPos,
                       std::shared_ptr<RawStream> Output) {
  generateHeader(InputFilename, Output);
  generateEnterNamespaces(Namespaces, Output);
  generateArrayAccessorHeader(FunctionName, Output);
  Output->puts(
      ";\n"
      "\n");
  generateArraySizeAccessorHeader(FunctionName, Output);
  Output->puts(
      ";\n"
      "\n");
  generateExitNamespaces(Namespaces, Output);
  Output->freeze();
}

void generateArrayImpl(const char* InputFilename,
                       std::vector<const char*>& Namespaces,
                       const char* FunctionName,
                       std::shared_ptr<ReadCursor> ReadPos,
                       std::shared_ptr<RawStream> Output) {
  generateHeader(InputFilename, Output);
  Output->puts(
      "#include \"utils/Defs.h\"\n"
      "\n"
      "namespace {\n"
      "\n"
      "const uint8_t ");
  Output->puts(FunctionName);
  Output->puts("Array[] = {\n");
  char Buffer[256];
  while (!ReadPos->atEof()) {
    uint8_t Byte = ReadPos->readByte();
    size_t Address = ReadPos->getCurByteAddress();
    if (Address > 0 && Address % BYTES_PER_LINE_IN_WASM_DEFAULTS == 0)
      Output->putc('\n');
    sprintf(Buffer, " %u", Byte);
    Output->puts(Buffer);
    if (!ReadPos->atEof())
      Output->putc(',');
  }
  Output->puts(
      "};\n"
      "\n"
      "}  // end of anonymous namespace\n"
      "\n");
  generateEnterNamespaces(Namespaces, Output);
  generateArrayAccessorHeader(FunctionName, Output);
  Output->puts(" { return ");
  Output->puts(FunctionName);
  Output->puts(
      "Array; }\n"
      "\n");
  generateArraySizeAccessorHeader(FunctionName, Output);
  Output->puts(" { return size(");
  Output->puts(FunctionName);
  Output->puts(
      "Array); }\n"
      "\n");
  generateExitNamespaces(Namespaces, Output);
  Output->freeze();
}

}  // end of anonymous namespace

int main(int Argc, const char* Argv[]) {
  bool MinimizeBlockSize = false;
  bool Verbose = false;
  bool TraceFlatten = false;
  bool TraceWrite = false;
  bool TraceTree = false;
  const char* FunctionName = nullptr;
  bool HeaderFile;
  {
    ArgsParser Args("Converts compression algorithm from text to binary");

    ArgsParser::RequiredCharstring AlgorithmFlag(AlgorithmFilename);
    Args.add(AlgorithmFlag.setShortName('a')
                 .setLongName("algorithm")
                 .setOptionName("ALGORITHM")
                 .setDescription(
                     "Use algorithm in ALGORITHM file "
                     "to parse text file"));

    ArgsParser::Bool ExpectFailFlag(ExpectExitFail);
    Args.add(ExpectFailFlag.setDefault(false)
                 .setLongName("expect-fail")
                 .setDescription("Succeed on failure/fail on success"));

    ArgsParser::Bool MinimizeBlockFlag(MinimizeBlockSize);
    Args.add(MinimizeBlockFlag.setShortName('m')
                 .setLongName("minimize")
                 .setDescription(
                     "Minimize size in binary file "
                     "(note: runs slower)"));

    ArgsParser::RequiredCharstring InputFlag(InputFilename);
    Args.add(InputFlag.setOptionName("INPUT")
                 .setDescription("Text file to convert to binary"));

    ArgsParser::OptionalCharstring OutputFlag(OutputFilename);
    Args.add(OutputFlag.setShortName('o')
                 .setLongName("output")
                 .setOptionName("OUTPUT")
                 .setDescription("Generated binary file"));

    ArgsParser::Bool VerboseFlag(Verbose);
    Args.add(
        VerboseFlag.setToggle(true)
            .setShortName('v')
            .setLongName("verbose")
            .setDescription("Show progress and tree written to binary file"));

    ArgsParser::Bool TraceFlattenFlag(TraceFlatten);
    Args.add(TraceFlattenFlag.setLongName("verbose=flatten")
                 .setDescription("Show how algorithms are flattened"));

    ArgsParser::Bool TraceWriteFlag(TraceWrite);
    Args.add(TraceWriteFlag.setLongName("verbose=write")
                 .setDescription("Show how binary file is encoded"));

    ArgsParser::Bool TraceTreeFlag(TraceTree);
    Args.add(TraceTreeFlag.setLongName("verbose=tree")
                 .setDescription(
                     "Show tree being written while writing "
                     "(implies --verbose=write)"));

    ArgsParser::Bool TraceParserFlag(TraceParser);
    Args.add(TraceParserFlag.setLongName("verbose=parser")
                 .setDescription(
                     "Show parsing of algorithm (defined by option -a)"));

    ArgsParser::Bool TraceLexerFlag(TraceLexer);
    Args.add(
        TraceLexerFlag.setLongName("verbose=lexer")
            .setDescription("Show lexing of algorithm (defined by option -a)"));

    ArgsParser::OptionalCharstring FunctionNameFlag(FunctionName);
    Args.add(FunctionNameFlag.setShortName('f')
                 .setLongName("function")
                 .setOptionName("Name")
                 .setDescription(
                     "Generate c++ source code to implement an array "
                     "containing the binary encoding, with accessors "
                     "Name() and NameSize()"));

    ArgsParser::Bool HeaderFileFlag(HeaderFile);
    Args.add(HeaderFileFlag.setLongName("header").setDescription(
        "Generate header version of c++ source instead "
        "of implementatoin file (only applies when "
        "'--function Name' is specified)"));

    switch (Args.parse(Argc, Argv)) {
      case ArgsParser::State::Good:
        break;
      case ArgsParser::State::Usage:
        return exit_status(EXIT_SUCCESS);
      default:
        fprintf(stderr, "Unable to parse command line arguments!\n");
        return exit_status(EXIT_FAILURE);
    }

    // Be sure to update implications!
    if (TraceTree)
      TraceWrite = true;
  }
  if (Verbose)
    fprintf(stderr, "Reading input: %s\n", InputFilename);
  std::shared_ptr<SymbolTable> InputSymtab = parseFile(InputFilename);
  if (!InputSymtab)
    return exit_status(EXIT_FAILURE);
  if (Verbose) {
    fprintf(stderr, "Read tree:\n");
    TextWriter Writer;
    Writer.write(stderr, InputSymtab.get());
  }
  std::shared_ptr<IntStream> IntSeq = std::make_shared<IntStream>();
  FlattenAst Flattener(IntSeq, InputSymtab);
  if (TraceFlatten) {
    auto Trace = std::make_shared<TraceClass>("flatten");
    Flattener.setTrace(Trace);
    Flattener.setTraceProgress(true);
  }
  if (!Flattener.flatten()) {
    fprintf(stderr, "Problems flattening read tree!\n");
    return exit_status(EXIT_FAILURE);
  }
  if (Verbose)
    fprintf(stderr, "Reading algorithms file: %s\n", AlgorithmFilename);
  std::shared_ptr<SymbolTable> AlgSymtab = parseFile(AlgorithmFilename);
  if (!AlgSymtab)
    return exit_status(EXIT_FAILURE);
  if (Verbose && strcmp(OutputFilename, "-") != 0)
    fprintf(stderr, "Opening output file: %s\n", OutputFilename);
  std::shared_ptr<RawStream> Output = getOutput();
  if (Output->hasErrors()) {
    fprintf(stderr, "Problems opening output file: %s", OutputFilename);
    return exit_status(EXIT_FAILURE);
  }
  if (Verbose)
    fprintf(stderr, "Writing file: %s\n", OutputFilename);
  std::shared_ptr<decode::Queue> OutputStream;
  std::shared_ptr<ReadCursor> ReadPos;
  if (FunctionName == nullptr)
    OutputStream = std::make_shared<WriteBackedQueue>(Output);
  else {
    OutputStream = std::make_shared<Queue>();
    ReadPos = std::make_shared<ReadCursor>(StreamType::Byte, OutputStream);
  }
  std::shared_ptr<Writer> Writer = std::make_shared<StreamWriter>(OutputStream);
  if (TraceTree) {
    auto Tee = std::make_shared<TeeWriter>();
    Tee->add(std::make_shared<InflateAst>(), false, true, false);
    Tee->add(Writer, true, false, true);
    Writer = Tee;
  }
  Writer->setMinimizeBlockSize(MinimizeBlockSize);
  auto Reader = std::make_shared<IntReader>(IntSeq, *Writer, AlgSymtab);
  if (TraceWrite) {
    auto Trace = std::make_shared<TraceClass>("write");
    Trace->setTraceProgress(true);
    Reader->setTrace(Trace);
    Writer->setTrace(Trace);
  }
  Reader->useFileHeader(InputSymtab->getInstalledHeader());
  Reader->algorithmStart();
  Reader->algorithmReadBackFilled();
  if (Reader->errorsFound()) {
    fprintf(stderr, "Problems while reading: %s\n", InputFilename);
    return exit_status(EXIT_FAILURE);
  }
  if (FunctionName != nullptr) {
    std::vector<const char*> Namespaces;
    Namespaces.push_back("wasm");
    Namespaces.push_back("decode");
    if (HeaderFile)
      generateArrayDecl(InputFilename, Namespaces, FunctionName, ReadPos,
                        Output);
    else
      generateArrayImpl(InputFilename, Namespaces, FunctionName, ReadPos,
                        Output);
  }
  return exit_status(EXIT_SUCCESS);
}
