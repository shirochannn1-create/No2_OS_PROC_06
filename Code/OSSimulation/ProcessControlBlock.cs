using System;
using System.Collections.Generic;

namespace OSSimulation.Models
{
    public enum ProcessState
    {
        New,
        Ready,
        Running,
        Waiting, 
        Terminated
    }

    public class ProcessControlBlock
    {
        public int ProcessID { get; private set; }
        public string ProcessName { get; set; }
        public ProcessState State { get; set; }

        public int ProgramCounter { get; set; }

        public int[] CpuRegisters { get; set; }

        public int Priority { get; set; }
        public int ArrivalTime { get; set; }
        public int BurstTime { get; set; }
        public int RemainingTime { get; set; }

        public int BaseRegister { get; set; } 
        public int LimitRegister { get; set; }

        public List<string> AllocatedIODevices { get; set; }

        public ProcessControlBlock(int pid, string name, int arrivalTime, int burstTime, int priority = 0)
        {
            ProcessID = pid;
            ProcessName = name;
            State = ProcessState.New;

            ProgramCounter = 0;
            CpuRegisters = new int[4]; 

            ArrivalTime = arrivalTime;
            BurstTime = burstTime;
            RemainingTime = burstTime; 
            Priority = priority;

            AllocatedIODevices = new List<string>();
        }

        public void SaveContext(int currentPC, int[] currentRegisters)
        {
            this.ProgramCounter = currentPC;
            Array.Copy(currentRegisters, this.CpuRegisters, currentRegisters.Length);
        }

        public override string ToString()
        {
            return $"[PID: {ProcessID} | {ProcessName}] State: {State}, PC: {ProgramCounter}, Remaining: {RemainingTime}ms";
        }
    }
}