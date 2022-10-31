using System;
using System.IO;
using System.Drawing;

public class FFTDataToImage
{
    static void Main(string[] args)
    {
        Console.Write("Input file?");
        //string filepath = Console.ReadLine();
        string[] allLines = File.ReadAllLines("build/PALout.txt"); //hardcoded for easy debugging
        



        

        string[] testLine = allLines[0].Split(',');
        Bitmap bmp = new Bitmap(testLine.Length, allLines.Length);
        for(int i = 0; i < allLines.Length; i++)
        {
            string[] line = allLines[i].Split(',');
            for(int j = 0; j < line.Length; j++)
            {
                int color = (int)(float.Parse(line[j]) * 255);
                bmp.SetPixel(j, i, Color.FromArgb(color, color, color));
            }
        }

        
        
        bmp.Save("build/PALout.png");
    }

    static float ReverseNumberInRange(float number, float min, float max)
    {
        return (max + min) - number;
    }

    
}