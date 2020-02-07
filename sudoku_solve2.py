import sys
import tkinter as tk
import tkinter.font as tkfont
import tkinter.filedialog as tkfile
import tkinter.messagebox as tkmessage
import argparse
import ctypes
import platform
import signal

def signal_handler(signal, frame):
    print("\nForced end\n")
    sys.exit(0)

class Persist(tk.Frame):
	SUBSIZE = 3
	X = False
	Window = False
	Debug = False
	Fsize = 14
	Data = None
	solve_lib = None
	s_lib={}
	Lib={}
	mode = "normal"
	
	@classmethod
	def LibSet(cls):		
		for i in range(2,7):
			cls.Lib[i] = False
	
	@classmethod
	def LibUse(cls):
		if not cls.Lib[cls.SUBSIZE]:
			# Shared C library
			lib_base = "./" #location
			lib_base += "sudoku2_lib" # base name
			
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
	solution = False
	
	def __init__(self, master=None):
		super().__init__(master)
		self.SIZE = Persist.SUBSIZE * Persist.SUBSIZE
		self.TOTALSIZE = self.SIZE * self.SIZE
		self.master = master
		self.font = tkfont.Font(weight="bold",size=Persist.Fsize)
		#self.pack()

		self.X = tk.BooleanVar()
		self.X.set(Persist.X)
		self.Window = tk.BooleanVar()
		self.Window.set(Persist.Window)
		self.Debug = tk.BooleanVar()
		self.Debug.set(Persist.Debug)

		self.create_widgets()
		self.create_menu()
		if ( Persist.Data ):
			self.SetData()
		else:
			self.BadData()
		self.win.update()
		self.set_win_sizes()

	def set_win_sizes( self ):
		# needed for popup
		self.tilex=self.but[0][0].winfo_width()
		self.tiley=self.but[0][0].winfo_height()
		self.popx = self.tilex*Persist.SUBSIZE
		self.popy = self.tiley*Persist.SUBSIZE
		self.winx = self.win.winfo_screenwidth()
		self.winy = self.win.winfo_screenheight()
		
	def	UnRed( self ):
		if self.solution:
			# need to clear old solution
			for i in range(self.SIZE):
				for j in range(self.SIZE):
					self.but[i][j].configure(fg="black")
			self.solution = False


	def set_square(self,i,j,n):
		self.but[i][j].configure(text=n)
		self.sq_popup_done( i , j )
		self.UnRed()
		
	def sq_popup_done( self, i, j ):
		self.pop.destroy()
		self.but[i][j].configure(relief="raised")
		self.status.configure(text="Edit mode")
			
	
	def sq_popup( self,i,j):
		self.but[i][j].configure(relief="sunken")
		self.pop=tk.Toplevel()
		
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
				tk.Button(self.pop,text=str(n),borderwidth=3,height=1,width=1,font=self.font,command=lambda i=i,j=j,n=str(n): self.set_square(i,j,n)).grid(row=si,column=sj)
		tk.Button(self.pop,text="Clear",borderwidth=3,height=2,font=self.font,command=lambda i=i,j=j,n=" ": self.set_square(i,j,n)).grid(columnspan=Persist.SUBSIZE,sticky="EW")
		tk.Button(self.pop,text="Back",borderwidth=3,height=2,font=self.font,command=lambda i=i,j=j: self.sq_popup_done(i,j)).grid(columnspan=Persist.SUBSIZE,sticky="EW")
		self.pop.grab_set()

	def Clear(self):
		self.status.configure(text="Clearing...")
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(text=" ")
		self.status.configure(text="Cleared")
		self.UnRed()
				
	def Solve(self):
		self.status.configure(text="Solving...")
		self.master.update()
		arr = (ctypes.c_int * self.TOTALSIZE)(-1)
		k = 0
		self.solution = True
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				arr[k] = -1 # default blank
				t = self.but[i][j].cget('text')
				if t != " ":
					arr[k] = int(t)-1 # 0-based values for squares
					self.but[i][j].configure(fg="red")
					#print(i,j,k,arr[k])
				#print("{} -> {}".format(k,arr[k]))
				k += 1

		x = 1 if Persist.X else 0
		w = 1 if Persist.Window else 0
		d = 1 if Persist.Debug else 0

		sol = Persist.solve_lib.Solve(x,w,d,arr)
		while True:
			if sol == 0:
				self.status.configure(text="Not solvable")
				for i in range(self.SIZE):
					for j in range(self.SIZE):
						if self.but[i][j].cget('fg') != 'red':
							self.but[i][j].configure(text=" ")
				self.master.update()
				break
			if sol == 1:
				self.status.configure(text="Successfully solved")
				k = 0
				for i in range(self.SIZE):
					for j in range(self.SIZE):
						if arr[k] >= 0 :
							self.but[i][j].configure(text=str(arr[k]+1)) # 1-based text values
							if self.but[i][j].cget('fg') == 'blue':
								self.but[i][j].configure(fg='black')
						else:
							self.but[i][j].configure(text=" ")
						k += 1
				self.master.update()
				break
			if sol < 0:
				self.status.configure(text="<"+str(-sol)+">  Still solving...")
				k = 0
				for i in range(self.SIZE):
					for j in range(self.SIZE):
						if self.but[i][j].cget('fg') != 'red':
							self.but[i][j].configure(fg='blue')
						if arr[k] >= 0 :
							self.but[i][j].configure(text=str(arr[k]+1)) # 1-based text values
						else:
							self.but[i][j].configure(text=" ")
						k += 1
			self.master.update()
			sol = Persist.solve_lib.Resume()

	def Test( self ):
		if not self.just_test():
			tkmessage.showinfo("Position test","Not valid")

	def just_test(self):
		arr = (ctypes.c_int * self.TOTALSIZE)(-1)
		k = 0
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				arr[k] = -1 # default blank
				t = self.but[i][j].cget('text')
				if t != " ":
					arr[k] = int(t)-1 # 0-based values for squares
					self.but[i][j].configure(fg="red")
				k += 1

		x = 1 if Persist.X else 0
		w = 1 if Persist.Window else 0
		d = 1 if Persist.Debug else 0

		return (Persist.solve_lib.Test(x,w,d,arr)==1)
	
	def Quit(self):
		sys.exit()
		self.master.quit()
	
	def create_widgets(self):
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
						self.but[i][j] = tk.Button(f,text=" ",borderwidth=3,height=1,width=1,font=self.font,command=lambda i=i,j=j: self.sq_popup(i,j))
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
				self.but[i][j].configure(text=str(1+(i+j)%self.SIZE))

	def SetData( self ):
		for i in range(self.SIZE):
			for j in range(self.SIZE):
				self.but[i][j].configure(fg='black')
				s = int(Persist.Data[i][j])
				if s == "0":
					s = " "
				self.but[i][j].configure(text=s) # 1-based text values

			
	def about(self):
		print("Sudoku Solve by Paul Alfille 2020")
		
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

	def fsize( self, f ):
		if ( f != Persist.Fsize ) :
			Persist.Fsize = f
			self.master.destroy()

	def create_menu(self):
		self.menu = tk.Menu(self.master,tearoff=0)

		self.filemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="File",menu=self.filemenu,font=self.font)
		self.filemenu.add_command(label="Load",command=self.Load,font=self.font)
		self.filemenu.add_command(label="Save",command=self.Save,font=self.font)
		self.filemenu.add_command(label="Solve",command=self.Solve,font=self.font)
		self.filemenu.add_command(label="Test",command=self.Test,font=self.font)
		self.filemenu.add_command(label="Clear",command=self.Clear,font=self.font)
		self.filemenu.add_command(label="Exit",command=self.Quit,font=self.font)

		self.sizemenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Size",menu=self.sizemenu,font=self.font)
		self.ss_choose = tk.IntVar()
		ss_choose = Persist.SUBSIZE
		for ss in range(2,7):
			self.sizemenu.add_radiobutton(label=str(ss*ss)+"x"+str(ss*ss), value=ss, variable=self.ss_choose, command=self.Size,font=self.font)

		self.optmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Options",menu=self.optmenu,font=self.font)
		self.optmenu.add_checkbutton(label="X pattern",onvalue=True,offvalue=False,variable=self.X,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Window pane",onvalue=True,offvalue=False,variable=self.Window,font=self.font,command=self.Option)
		self.optmenu.add_checkbutton(label="Debugging data",onvalue=True,offvalue=False,variable=self.Debug,font=self.font,command=self.Option)
		self.fontmenu = tk.Menu(self.optmenu,tearoff=0)
		self.optmenu.add_cascade(label="Font size",menu=self.fontmenu,font=self.font)
		for ff in [6,8,10,14,18,22,26]:
			self.fontmenu.add_command(label=str(ff), font=self.font, command=lambda ff=ff: self.fsize(ff))

		self.helpmenu = tk.Menu(self.menu,tearoff=0)
		self.menu.add_cascade(label="Help",menu=self.helpmenu,font=self.font)
		self.helpmenu.add_command(label="About",command=self.about,font=self.font)

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
							Persiste.Window = Window
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

def Libs():
	# returns a dict
	s_lib={}
	
	for ss in range(2,7):
		# Shared C library
		lib_base = "./" #location
		lib_base += "sudoku2_lib" # base name
		
		lib_base += str(ss*ss)	

		# get the right filename
		if platform.uname()[0] == "Windows":
			lib_base += ".dll" 
		if platform.uname()[0] == "Linux":
			lib_base += ".so" 
		else:
			lib_base += ".dylib" 

		# load library
		global solve_lib
		s_lib[ss] = ctypes.cdll.LoadLibrary(lib_base)
	
	return s_lib

def main(args):
	# keyboard interrupt
	signal.signal(signal.SIGINT, signal_handler)

	# set up library dist
	#global solve_lib
	#s_lib = Libs()
	Persist.LibSet()
	 
	while True:
		# load library
		Persist.LibUse()
		Sudoku(master=tk.Tk()).mainloop()

if __name__ == "__main__":
	# execute only if run as a script
	sys.exit(main(sys.argv))
