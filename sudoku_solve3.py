import sys
import tkinter as tk
import tkinter.font as tkfont
import tkinter.filedialog as tkfile
import tkinter.messagebox as tkmessage
import argparse
import ctypes
import platform
import signal
import tkinter.scrolledtext as scrolledtext

def signal_handler(signal, frame):
    print("\nForced end\n")
    sys.exit(0)

class Persist(tk.Frame):
	SUBSIZE = 3
	X = False
	Window = False
	Debug = False
	Fsize = 14
	GameStatus="None"
	Data = None
	solve_lib = None
	s_lib={}
	Lib={}
	mode = "normal"
	Legal=True
	
	@classmethod
	def LibSet(cls):		
		for i in range(2,7):
			cls.Lib[i] = False
	
	@classmethod
	def LibUse(cls):
		if cls.solve_lib:
			# kill any running solver before switching to a new library
			cls.solve_lib.ThreadlistKill()
		
		if not cls.Lib[cls.SUBSIZE]:
			# Shared C library
			lib_base = "./" #location
			lib_base += "sudoku3_lib" # base name
			
			lib_base += str(cls.SUBSIZE*cls.SUBSIZE)	

			# get the right filename
			if platform.uname()[0] == "Windows":
				lib_base += ".dll" 
			if platform.uname()[0] == "Linux":
				lib_base += ".so" 
			else:
				lib_base += ".dylib" 

			# load library
			cls.s_lib[cls.SUBSIZE] = ctypes.cdll.LoadLibrary(lib_base)
			
			cls.Lib[cls.SUBSIZE] = True
	
		cls.solve_lib = cls.s_lib[cls.SUBSIZE]


class Sudoku(tk.Frame):
	color=["dark blue","yellow"]
	after = None
	candidate = None
	statustext = ""
	
	def __init__(self, master=None):
		super().__init__(master)

		self.SIZE = Persist.SUBSIZE * Persist.SUBSIZE
		self.TOTALSIZE = self.SIZE * self.SIZE
		self.arr = (ctypes.c_int * self.TOTALSIZE)(-1)

		self.master = master
		self.master.title("Sudoku")
		
		self.option_setup()
		
		self.Widget()
		self.Menu()
		if ( Persist.Data ):
			self.SetData()
		else:
			self.BadData()
		self.win.update()
		self.set_win_sizes()
		self.Lib()
		self.SetBoard()
		
	def Lib(self): 
		# library setup
		x = 1 if Persist.X else 0
		w = 1 if Persist.Window else 0
		d = 1 if Persist.Debug else 0
		Persist.solve_lib.Setup( x,w,d,self.arr )

	def set_win_sizes( self ):
		# needed for popup
		self.tilex=self.but[0][0].winfo_width()
		self.tiley=self.but[0][0].winfo_height()
		self.popx = self.tilex*Persist.SUBSIZE
		self.popy = self.tiley*Persist.SUBSIZE
		self.winx = self.win.winfo_screenwidth()
		self.winy = self.win.winfo_screenheight()
		
	def	UnRed( self ):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(fg="black")


	def set_square(self,i,j,n):
		self.set_array( i, j, n )
		t = str(n) if n > 0 else " "
		self.but[i][j].configure(text=t)
		self.popup_done( i , j )
		self.SetBoard()
		
	def popup_done( self, i, j ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
		self.Status()
		
	def Status( self, stat = None ):
		if not stat:
			stat = Persist.solve_lib.GetStatus()
		text = ["Setup","Error","Illegal","Legal","Working","Unsolvable","Solvable","Unique","Not unique"][stat]
		if text != self.statustext:
			self.statustext = text
			self.status.configure( text=text )
			self.master.title(text)
			self.winfo_toplevel().title(text)
		return stat		
			
	def popup_force_done( self, i, j, force ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
		self.Status()
		self.Popup(i,j,not force)
	
	def Popup( self,i,j,force):
		self.but[i][j].configure(relief="sunken")
		self.pop=tk.Toplevel()
		
		show = False
		if Persist.Legal:
			goodlist = self.Available(i,j)
		else:
			goodlist = [x+1 for x in range(self.SIZE)]
		t = [0 for n in range(self.SIZE+1)]
		
		# figure location
		self.win.update()
		x = self.but[i][j].winfo_rootx()+self.tilex
		y = self.but[i][j].winfo_rooty()
		if x+self.popx > self.winx:
			x -= self.tilex+self.popx
		if x<0:
			x=0
		if y+self.popy > self.winy:
			y = self.winy - self.popy
		self.pop.geometry('+%d+%d' % (x,y) )
		
		for si in range(Persist.SUBSIZE):
			for sj in range(Persist.SUBSIZE):
				n = si*Persist.SUBSIZE+sj+1				
				t[n]=tk.Button(self.pop,text=str(n),borderwidth=3,height=1,width=1,font=self.font, state='normal' if (n in goodlist) or not force else 'disabled', command=lambda i=i,j=j,n=n: self.set_square(i,j,n))
				t[n].grid(row=si+1,column=sj)

		if 0 in goodlist:
			tk.Button(self.pop,text="force" if force else "unforce",borderwidth=3,height=1,font=self.font,command=lambda i=i,j=j: self.popup_force_done(i,j,force)).grid(row=0,columnspan=Persist.SUBSIZE,sticky="EW")

		tk.Button(self.pop,text="Clear",borderwidth=3,height=1,font=self.font,command=lambda i=i,j=j,n=0: self.set_square(i,j,n)).grid(columnspan=Persist.SUBSIZE,sticky="EW")
		tk.Button(self.pop,text="Back",borderwidth=3,height=1,font=self.font,command=lambda i=i,j=j: self.popup_done(i,j)).grid(columnspan=Persist.SUBSIZE,sticky="EW")

		self.pop.grab_set()

	def Clear(self):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(text=" ")
				self.set_array(i,j,0)
		self.Status()
		self.SetBoard()
				
	def SetBoard(self): # set library representation
		if self.candidate:
			self.UnRed()
		self.candidate = None

		if self.after:
			self.master.after_cancel(self.after)
		self.Status( stat = Persist.solve_lib.SetBoard() )
		
	def set_array(self, i, j, val ):
		self.arr[i*self.SIZE+j] = val
	
	def GetBoard(self):
		Persist.solve_lib.GetBoard()
		k = 0
		if self.candidate:
			for i in range(self.SIZE):
				for j in range(self.SIZE):
					if self.arr[k] > 0 :
						if self.candidate[k]>0:
							self.but[i][j].configure(text=str(self.arr[k]),fg='red') # 1-based text values
						elif self.lastboard[k] != self.arr[k]:
							self.but[i][j].configure(text=str(self.arr[k]),fg='black') # 1-based text values
							self.lastboard[k] = self.arr[k]
					else:
						if self.lastboard[k] != 0:
							self.but[i][j].configure(text=" ")
							self.lastboard[k] = 0
					k += 1
		else:
			for i in range(self.SIZE):
				for j in range(self.SIZE):
					if self.arr[k] > 0 :
						self.but[i][j].configure(text=str(self.arr[k]),fg='black') # 1-based text values
					else:
						self.but[i][j].configure(text=" ",fg='black')
					k += 1
			self.master.update()
		stat = self.Status()
		return stat

	def solving( self ):
		g = self.GetBoard()
		if g in [ 4 ]:
			self.after = self.master.after(1000,self.solving)

	def Solve(self):
		self.candidate = self.arr[:]
		self.lastboard = self.arr[:]
		self.Status(stat=Persist.solve_lib.Solve())
		self.after = self.master.after( 500, self.solving )		

	def Available(self,testi,testj):
		ret = (ctypes.c_int * self.SIZE)(-1)
		self.Status(stat=Persist.solve_lib.GetAvailable(testi,testj,ret))
		return ret
		
	def Unique(self):
		return Persist.solve_lib.TestUnique()
	
	def Quit(self):
		sys.exit()
		self.master.quit()
	
	def Widget(self):
		self.win = tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		self.win.pack(side="top")
		self.buttons=tk.Frame(self.master,borderwidth=2,relief="flat",background="white")
		if Persist.SUBSIZE > 2:
			tk.Label(self.buttons,text=" {0}x{0} ".format(self.SIZE),relief="sunken",anchor="c",font=self.font).pack(side="left",fill=tk.Y)
		tk.Button(self.buttons,text="Solve",command=self.Solve,font=self.font).pack(side="left")
		tk.Button(self.buttons,text="Clear",command=self.Clear,font=self.font).pack(side="left")
		tk.Button(self.buttons,text="Exit",command=self.Quit,font=self.font).pack(side="left")
		self.status = tk.Label(self.buttons,text="Edit mode",relief="sunken",anchor="e")
		self.status.pack(side="left",fill="both",expand=1)
		self.buttons.pack(side="bottom",fill=tk.X)
	
		self.but = [[0 for i in range(self.SIZE)] for j in range(self.SIZE)]
		for si in range(Persist.SUBSIZE):
			for sj in range(Persist.SUBSIZE):
				f = tk.Frame(self.win,background=self.color[(si+sj)%2],borderwidth=2,relief="flat")
				f = tk.Frame(self.win,background=self.color[(si+sj)%2],borderwidth=2,relief="flat")
				f.grid(row=si,column=sj)
				for ssi in range(Persist.SUBSIZE):
					for ssj in range(Persist.SUBSIZE):
						i = si*Persist.SUBSIZE+ssi
						j = sj*Persist.SUBSIZE+ssj
						self.but[i][j] = tk.Button(f,text=" ",fg='black',borderwidth=3,height=1,width=1,font=self.font,command=lambda i=i,j=j: self.Popup(i,j,True))
						self.but[i][j].grid(row=ssi, column=ssj)
						if Persist.X and ((i==j) or (i == self.SIZE-j-1)):
							self.but[i][j].configure(background="light yellow")
						if Persist.Window:
							if (i % (Persist.SUBSIZE+1) > 0) and (j % (Persist.SUBSIZE+1) > 0):
								self.but[i][j].configure(background="aquamarine")
								if Persist.X and ((i==j) or (i == self.SIZE-j-1)):
									self.but[i][j].configure(background="pale green")
								
	def BadData( self ):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				v = 1+(i+j)%self.SIZE
				self.but[i][j].configure(text=str(v))
				self.set_array( i, j, v )

	def SetData( self ):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				s = str(Persist.Data[i][j])
				if s == '0':
					s = ' '
				self.but[i][j].configure(text=s) # 1-based text values
				self.set_array( i, j, Persist.Data[i][j] )
		Persist.Data = None
			
	def About(self):
		print("Sudoku Solve by Paul Alfille 2020")
		
	def Help( self ):
		self.helpwin = tk.Toplevel()
		self.helptext = scrolledtext.ScrolledText(self.helpwin, undo=False)
		tk.Button(self.helpwin,text="Ok",command=lambda: self.helpwin.destroy()).pack(side="bottom")
		self.helptext.insert("end",'''\
Sudoku count is a suite of programs to investigate soduku solving.

This program, sudoku_solve, is a sudoku board builder and solver.
	The size of the board can be varied, from 4X4 to 36x36 (although the larger sizes are difficult to use or solve).
	Each square can be assigned (by default only allowable values are enabled, but others can be forced)
	The status line will show the current state of the board.
	Solving can take a while for larger boards, but intermediate steps will be shown.
	It is possible to interrupt solving be trying to assign a square.
	
Sudoku, for those who are not familiar, is a puzzle to assign values to each square in a grid such that no repeat values are in each row, column or subsquare
	Harder constraints can be added (Window and X in the options).

The structure of the program (for those who care) is python3 interface, using tkinter with a C library backend for faster calculation
		''')
		self.helptext.configure(state="disabled")
		self.helptext.pack(expand=True,fill="both")
		
	def Size(self) :
		if ( self.ss_choose != Persist.SUBSIZE ):
			Persist.SUBSIZE = self.ss_choose.get()
			self.master.destroy()
	
	def Option(self):
		if Persist.X != self.X.get():
			Persist.X = self.X.get()
			self.master.destroy()
		if Persist.Window != self.Window.get():
			Persist.Window = self.Window.get()
			self.master.destroy()
		Persist.Debug = self.Debug.get()
		Persist.Legal = self.Legal.get()

	def option_setup(self):
		# match with Option(), set in Menu()
		self.font = tkfont.Font(weight="bold",size=Persist.Fsize)
		#self.pack()

		self.X = tk.BooleanVar()
		self.X.set(Persist.X)
		
		self.Window = tk.BooleanVar()
		self.Window.set(Persist.Window)
		
		self.Debug = tk.BooleanVar()
		self.Debug.set(Persist.Debug)
		
		self.Legal = tk.BooleanVar()
		self.Legal.set(Persist.Legal)
		
	def fsize( self, f ):
		if ( f != Persist.Fsize ) :
			Persist.Fsize = f
			self.master.destroy()

	def set_status( self, s ):
		Persist.GameStatus = s
		self.Status()

	def Menu(self):
		self.menu = tk.Menu(self.master,tearoff=0)

		self.filemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="File",menu=self.filemenu,font=self.font)
		self.filemenu.add_command(label="Load",command=self.Load,font=self.font)
		self.filemenu.add_command(label="Save",command=self.Save,font=self.font)
		self.filemenu.add_command(label="Solve",command=self.Solve,font=self.font)
		self.filemenu.add_command(label="Clear",command=self.Clear,font=self.font)
		self.filemenu.add_command(label="Exit",command=self.Quit,font=self.font)

		self.sizemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Size",menu=self.sizemenu,font=self.font)
		self.ss_choose = tk.IntVar()
		ss_choose = Persist.SUBSIZE
		for ss in range(2,7):
			self.sizemenu.add_radiobutton(label=("> " if ss == Persist.SUBSIZE else "")+str(ss*ss)+"x"+str(ss*ss), value=ss, variable=self.ss_choose, command=self.Size,font=self.font)

		self.optmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Options",menu=self.optmenu,font=self.font)
		self.optmenu.add_checkbutton(label="X pattern",onvalue=True,offvalue=False,variable=self.X,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Window pane",onvalue=True,offvalue=False,variable=self.Window,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Debugging data",onvalue=True,offvalue=False,variable=self.Debug,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Legal choices",onvalue=True,offvalue=False,variable=self.Legal,font=self.font,command=self.Option)

		self.fontmenu = tk.Menu(self.optmenu,tearoff=0)
		self.optmenu.add_cascade(label="Font size",menu=self.fontmenu,font=self.font)
		for ff in [6,8,10,14,18,22,26]:
			self.fontmenu.add_command(label=("> " if ff==Persist.Fsize else "")+str(ff),font=self.font, command=lambda ff=ff: self.fsize(ff))

		self.statusmenu = tk.Menu(self.optmenu,tearoff=0)
		self.optmenu.add_cascade(label="Game state status",menu=self.statusmenu,font=self.font)
		for ss in ["None","Unique"]:
			self.statusmenu.add_command(label=("> " if ss==Persist.GameStatus else "")+ss, font=self.font, command=lambda ss=ss: self.set_status(ss))

		self.helpmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Help",menu=self.helpmenu,font=self.font)
		self.helpmenu.add_command(label="About",command=self.About,font=self.font)
		self.helpmenu.add_command(label="Help",command=self.Help,font=self.font)

		self.master.config(menu=self.menu)
			
	def Load( self ):
		Lfile = tkfile.askopenfile(mode="r",filetypes=[("Comma-separated-values","*.csv"),("All files","*.*")],title="Load a sudoku",parent=self.master)
		if Lfile:
			try:
				i = 0
				Window = False
				X = False
				for line in Lfile:
					if '#' in line:
						[line,comment]=line.split('#')
						if "=" in comment:
							[var,val] = line.split("=")
						else:
							[var,val] = [comment,"true"]
						if var == "window":
							Window = (val=="true")
						if var == "X":
							X = (val=="true")
					if ',' in line:
						v = line.split(',')
						if i == 0 :
							#first line, check size
							Lsize = len(v)
							if Lsize not in [4,9,16,25,36]:
								tkmessage.showerror("File error","Not a recognized size")
								Persist.Data=None
								break
							Persist.Data = [[0 for i in range(self.SIZE)] for j in range(self.SIZE)]

						else:
							if len(v) != Lsize:
								tkmessage.showerror("File error","Value lists not the same size")
								Persist.Data=None
								break
						Persist.Data[i] = [int(x) if x.isnumeric() else 0 for x in list(map(lambda s:s.strip(),v)) ]
						if max(Persist.Data[i]) > Lsize or min(Persist.Data[i]) < 0 :
								tkmessage.showerror("File error","Value out of range")
								Persist.Data=None
								break
						i += 1
						if i == Lsize:
							#done
							Persist.X = X
							Persist.Window = Window
							Persist.SUBSIZE = [x*x for x in range(7)].index(Lsize)
							self.master.destroy()
				Lfile.close()
					
			except UnicodeDecodeError:
				tkmessage.showerror("Unreadable","File contains unreadable characters")
				Persist.Data=None
				return	

	def Save( self ):
		filename = tkfile.asksaveasfilename(filetypes=[("Comma-separated-values","*.csv"),("All files","*.*")],title="Save this sudoku board",parent=self.master)
		if filename:
			with open(filename,'w') as Sfile:
				if Persist.Window:
					Sfile.write("# window\n")
				if Persist.X:
					Sfile.write("# X\n")
				Sfile.write("\n".join([",".join([self.but[i][j].cget("text") for j in range(self.SIZE)]) for i in range(self.SIZE)])+"\n")

def main(args):
	# keyboard interrupt
	signal.signal(signal.SIGINT, signal_handler)

	Persist.LibSet()
	 
	while True:
		# load library
		Persist.LibUse()
		Sudoku(master=tk.Tk()).mainloop()

if __name__ == "__main__":
	# execute only if run as a script
	sys.exit(main(sys.argv))
