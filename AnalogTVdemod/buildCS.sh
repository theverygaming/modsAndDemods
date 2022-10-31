#!/bin/bash
rm txtToImage.exe
mcs /reference:System.Drawing.dll txtToImage.cs
mono txtToImage.exe