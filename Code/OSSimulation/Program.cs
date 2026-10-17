using System;
using OSSimulation.Models;

namespace OSSimulation
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("--- MO PHONG HE DIEU HANH: KHOI TAO TIEN TRINH ---");

            ProcessControlBlock p1 = new ProcessControlBlock(1, "Chrome.exe", 0, 100);
            ProcessControlBlock p2 = new ProcessControlBlock(2, "Word.exe", 2, 80);

            Console.WriteLine(p1.ToString());
            Console.WriteLine(p2.ToString());

            Console.WriteLine("\n--- MO PHONG CONTEXT SWITCH ---");

            p1.State = ProcessState.Running;
            p1.ProgramCounter = 1048;
            int[] currentCpuRegisters = { 15, 0, 255, 10 };

            Console.WriteLine($"[He thong] P1 dang chay. PC = {p1.ProgramCounter}");

            Console.WriteLine("[He thong] Xay ra ngat! Luu trang thai P1...");
            p1.SaveContext(p1.ProgramCounter, currentCpuRegisters);
            p1.State = ProcessState.Waiting;

            p2.State = ProcessState.Running;
            Console.WriteLine("[He thong] Nap P2 vao CPU...");

            Console.WriteLine("\n--- TRANG THAI PCB SAU CONTEXT SWITCH ---");
            Console.WriteLine(p1.ToString());
            Console.WriteLine(p2.ToString());

            Console.ReadLine(); 
        }
    }
}